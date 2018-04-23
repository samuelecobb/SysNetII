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
#include "tokRingClient.h"

#define BUFF_SIZE 2048

NodeInfo			currNode;
int 				sockFd;
struct hostent 		*server;
char				*serverAddr = "127.0.0.1";
int					token = -1;
char				msgBuffer[BUFF_SIZE];

int main(int argc, char *argv[])
{

	if(argc<3)
	{
		fprintf(stderr, "Unrecognized command format: \n\nCall program with: ./TRClient <serverPortNum> <peerPortNum> <filename>\n");
		exit(0);
	}

	currNode.serverPort = atoi(argv[1]);
	currNode.peerPort = atoi(argv[2]);

	joinTokenRing();


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
	connectToServer(); //connect to next node

	bzero(msgBuffer, BUFF_SIZE);
	printf("Enter message: ");
	fgets(msgBuffer, BUFF_SIZE, stdin);
	clientSendMessage();

	clientListen();

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
