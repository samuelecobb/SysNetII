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
#define TRUE 0
#define FALSE 1

NodeInfo			currNode;
int 				sockFd;
struct hostent 		*server;
char				*serverAddr = "127.0.0.1";
char				msgBuffer[BUFF_SIZE];
bool				hasToken, programRun = TRUE;
FILE 				*fp;
pthread_mutex_t 	lock;


int main(int argc, char *argv[])
{

	if(argc<3)
	{
		fprintf(stderr, "Unrecognized command format: \n\nCall program with: ./TRClient <serverPortNum> <peerPortNum> <filename>\n");
		exit(0);
	}

	currNode.serverPort = atoi(argv[1]);
	currNode.peerPort = atoi(argv[2]);

	if((currNode.peerPort % 5) == 0)
	{
		hasToken = TRUE;
		fp = fopen(argv[3], "w");
		fclose(fp);
	}

	joinTokenRing();
	printMenu();

	pthread_t tid;
	pthread_create(&tid, NULL, tokenHandler, (void *)NULL);

	char opBuffer[4];
	OptionList currOption;

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
	currNode.serverPort = atoi(msgBuffer);
	printf("New server port is now: %d\n\n", currNode.serverPort);
	connectToServer(); //connect to next node

}

void clientSendMessage(void)
{
	if (sendto(sockFd, msgBuffer, strlen(msgBuffer), 0, (struct sockaddr *)&currNode.servAddr, sizeof(currNode.servAddr)) < 0)
			{
				fprintf(stderr, "Sendto failed\n");
			}
}

void clientListen(void)
{
	int recvLen, incomingPort;
	socklen_t addrlen = sizeof(currNode.servAddr);

	recvLen = recvfrom(sockFd, msgBuffer, BUFF_SIZE, 0, (struct sockaddr *)&currNode.servAddr, &addrlen);
	incomingPort = ntohs(currNode.servAddr.sin_port);
	printf("received %d bytes from port %d\n", recvLen, incomingPort);

	if (recvLen > 0)
	{
		msgBuffer[recvLen] = 0;
		printf("received message: %s", msgBuffer);
	}
}

void serverListen(void)
{
	int recvLen, incomingPort;
	socklen_t addrlen = sizeof(currNode.myAddr);

	recvLen = recvfrom(sockFd, msgBuffer, BUFF_SIZE, 0, (struct sockaddr *)&currNode.myAddr, &addrlen);
	incomingPort = ntohs(currNode.myAddr.sin_port);
	printf("received %d bytes from port %d\n", recvLen, incomingPort);

	if (recvLen > 0)
	{
		msgBuffer[recvLen] = 0;
		printf("received message: %s", msgBuffer);
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

	bcopy((char *)server->h_addr, (char *)&currNode.servAddr.sin_addr.s_addr, server->h_length);

	memset((char *) &currNode.servAddr, 0, sizeof(currNode.servAddr));
	currNode.servAddr.sin_family = AF_INET;
	currNode.servAddr.sin_port = htons(currNode.serverPort);
	if (inet_aton(serverAddr, &currNode.servAddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(0);
	}
}

void setupNode(void)
{
	memset((char *)&currNode.myAddr, 0, sizeof(currNode.myAddr));
	currNode.myAddr.sin_family = AF_INET;
	currNode.myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	currNode.myAddr.sin_port = htons(currNode.peerPort);

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
		if(hasToken == TRUE)
		{
			bzero(msgBuffer, BUFF_SIZE);
			msgBuffer[0] = 'T';
			clientSendMessage();
			hasToken == FALSE;
		}

		if(hasToken == FALSE)
		{
			bzero(msgBuffer, BUFF_SIZE);
			serverListen();
			if(msgBuffer[0] == 'T')
				hasToken == TRUE;
		}
	}
	
	
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
		printf("Recognized option: Read Message\n");
		return readOp;
	}
	if(input == 'w')
	{
		printf("Recognized option: Write Message\n");
		return writeOp;
	}
	if(input == 'l')
	{
		printf("Recognized option: List messages\n");
		return listOp;
	}
	if(input == 'e')
	{
		printf("Recognized option: Exit ring\n");
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
	printf("writing to bb\n");
}

void bbRead(void)
{
	printf("reading from bb\n");
}

void bbList(void)
{
	printf("listing messages from bb\n");
}

void bbExit(void)
{
	programRun = FALSE;
	hasToken = FALSE;
	printf("exiting token ring\n");
}