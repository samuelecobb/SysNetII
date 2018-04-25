/*
 * tokRingClient.c
 *
 *  Created on: Apr 21, 2018
 *      Author: samuel cobb
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include "tokRingClient.h"

#define BUFF_SIZE 2048
 #define MAX_LINE_SIZE 256
#define TRUE 0
#define FALSE 1

NodeInfo			currNode;
int 				sockFd;
struct hostent 		*server;
char				*serverAddr = "127.0.0.1";
char				*token = "T";
char				msgBuffer[BUFF_SIZE];
bool				hasToken;
bool				programRun;
FILE 				*fp;
pthread_mutex_t 	lock;
char*				fileName;


int main(int argc, char *argv[])
{

	if(argc<3)
	{
		fprintf(stderr, "Unrecognized command format: \n\nCall program with: ./TRClient <serverPortNum> <peerPortNum> <filename>\n");
		exit(0);
	}

	currNode.nextPort = atoi(argv[1]);
	currNode.myPort = atoi(argv[2]);

	if((currNode.myPort % 5) == 0)
	{
		hasToken = TRUE;
		printf("**********Winner! Token Received!**********\n");
	}
	else hasToken = FALSE;
	programRun = TRUE;
	fileName = argv[3];
	

	joinTokenRing();
	printMenu();

	pthread_t tid;
	pthread_create(&tid, NULL, tokenHandler, (void *)NULL);

	char opBuffer[4];
	OptionList currOption;

	printf("\nChoose option from menu above: ");
	fgets(opBuffer, 4, stdin);
	currOption = translateOption(opBuffer[0]);


	while(1)
	{
		if(hasToken == TRUE)
		{
			switch(currOption)
			{
				case writeOp:
					bbWrite();
					break;

				case readOp:
					bbRead();
					break;

				case listOp:
					bbList();
					break;

				case exitOp:
					bbExit();
					break;

				default:
					break;
			}
			printf("\nChoose option from menu above: ");
			fgets(opBuffer, 4, stdin);
			currOption = translateOption(opBuffer[0]);
		}
		
	}

	return 0;
}

void joinTokenRing(void)
{
	setupNode();
	connectToServer();

	bzero(msgBuffer, BUFF_SIZE);
	printf("Enter message: ");
	fgets(msgBuffer, BUFF_SIZE, stdin);

	clientSendMessage(); //send initial message
	clientListen(); //wait for response that includes port of next node
	currNode.nextPort = atoi(msgBuffer);
	printf("New server port is now: %d\n\n", currNode.nextPort);
	connectToServer(); //connect to next node

}

void clientSendMessage(void)
{
	if (sendto(sockFd, msgBuffer, strlen(msgBuffer), 0, (struct sockaddr *)&currNode.nextAddr, sizeof(currNode.nextAddr)) < 0)
			{
				fprintf(stderr, "Sendto failed\n");
			}
}

void clientListen(void) //only for initial communication with server program to setup ring
{
	int recvLen, incomingPort;
	socklen_t addrlen = sizeof(currNode.nextAddr);

	bzero(msgBuffer, BUFF_SIZE);
	recvLen = recvfrom(sockFd, msgBuffer, BUFF_SIZE, 0, (struct sockaddr *)&currNode.nextAddr, &addrlen);
	incomingPort = ntohs(currNode.nextAddr.sin_port);
	printf("received %d bytes from port %d\n", recvLen, incomingPort);

	if (recvLen > 0)
	{
		msgBuffer[recvLen] = 0;
		printf("received message: %s\n", msgBuffer);
	}
}

void connectToServer(void)
{
	server = gethostbyname(serverAddr);

	if(server == NULL)
	{
		fprintf(stderr, "ERROR: no such host\n");
		exit(0);
	}

	bcopy((char *)server->h_addr, (char *)&currNode.nextAddr.sin_addr.s_addr, server->h_length);

	memset((char *) &currNode.nextAddr, 0, sizeof(currNode.nextAddr));
	currNode.nextAddr.sin_family = AF_INET;
	currNode.nextAddr.sin_port = htons(currNode.nextPort);
	if (inet_aton(serverAddr, &currNode.nextAddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(0);
	}
}

void setupNode(void)
{
	memset((char *)&currNode.myAddr, 0, sizeof(currNode.myAddr));
	currNode.myAddr.sin_family = AF_INET;
	currNode.myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	currNode.myAddr.sin_port = htons(currNode.myPort);

	if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr, "Unable to create socket\n");
		exit(0);
	}

	if (bind(sockFd, (struct sockaddr*)&currNode.myAddr, sizeof(currNode.myAddr)) < 0)
	{
		fprintf(stderr, "Bind failed\n");
		exit(0);
	}
}

void* tokenHandler(void* param)
{
	while(programRun == TRUE)
	{
		pthread_mutex_lock(&lock);
		if(hasToken == TRUE)
		{
			if(sendMessage(token))
				hasToken = FALSE;
		}
		else if(hasToken == FALSE)
		{
			if(receiveMessage())
				hasToken = TRUE;

		}
		pthread_mutex_unlock(&lock);
	}

	return 0;
}

int sendMessage(char* message)
{
	if (sendto(sockFd, message, strlen(message), 0, (struct sockaddr *)&currNode.nextAddr, sizeof(currNode.nextAddr)) < 0)
	{
		return 0;		
	}
	else
		return 1;
}

int receiveMessage()
{
	int recvLen, incomingPort;
	socklen_t addrlen = sizeof(currNode.prevAddr);

	bzero(msgBuffer, BUFF_SIZE);
	recvLen = recvfrom(sockFd, msgBuffer, BUFF_SIZE, 0, (struct sockaddr *)&currNode.prevAddr, &addrlen);
	incomingPort = ntohs(currNode.prevAddr.sin_port);

	strtok(msgBuffer, "\n");

	if(msgBuffer[0] == 'T')
	{
		return 1;
	}
		

	else
		return 0;
}

void printMenu(void)
{
	printf("******************************************************************\n");
	printf(">>Enter w for write operation - Appends message to message board \n");
	printf(">>Enter r for read operation - Reads a message from message board\n");
	printf(">>Enter l for list operation - Displays number of messages\n");
	printf(">>Enter e for exit operation - Exits the program\n");
	printf("The message format is as follows:\n");
	printf("<message n=seq.no>\n");
	printf("<body>\n");
	printf("</message>\n");
	printf("******************************************************************\n");
}

OptionList translateOption(char input)
{
	if(input == 'r')
	{
		return readOp;
	}
	if(input == 'w')
	{
		return writeOp;
	}
	if(input == 'l')
	{
		return listOp;
	}
	if(input == 'e')
	{
		return exitOp;
	}
	else
	{
		printf("Unrecognized option: Please try again\n");
		return -1;
	}
}

void bbWrite(void)
{
	char fileMsg[BUFF_SIZE];
	int n = 1;
	char c;

	fp = fopen(fileName, "a+");

	for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n') // Increment count if this character is newline
            n = n + 1;

	printf("What do you want to write?\n");
	bzero(fileMsg, BUFF_SIZE);
	fgets(fileMsg, BUFF_SIZE, stdin);
	strtok(fileMsg, "\n");
	fprintf(fp, "<message n=%d>", n);
	fprintf(fp, "<%s>", fileMsg);
	fprintf(fp, "</message>\n");
	fclose(fp);
}

void bbRead(void)
{
	char messageNum[BUFF_SIZE];
	char str[MAX_LINE_SIZE];
	int count = 0;
	int line;

	printf("Which message do you want to read?\n");
	fgets(messageNum, BUFF_SIZE, stdin);
	line = atoi(messageNum);

	fp = fopen(fileName, "a+");

    while (fgets(str, MAX_LINE_SIZE, fp) != NULL) /* read a line */
    {
        if (count == (line-1))
        {
        	fclose(fp);
            printf("%s\n", str);
            return;
        }
        else
        {
            count++;
        }
    }
}

void bbList(void)
{
	int n = 0;
	char c;

	fp = fopen(fileName, "a+");

	for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n') // Increment count if this character is newline
            n = n + 1;
    printf("There are currently %d messages on the bulletin board\n", n);

}

void bbExit(void)
{
	programRun = FALSE;
	printf("exiting token ring\n");

}