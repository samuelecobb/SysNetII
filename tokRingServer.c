#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "tokRingServer.h"

ServerInfo			serverInfo;
int 				sockFd;
socklen_t addrlen = sizeof(serverInfo.clientAddr); 
char				msgBuffer[BUFF_SIZE];
int 				recvLen, incomingPort, numClients;

int main(int argc, char *argv[])
{
	serverInfo.port = atoi(argv[1]);
	numClients = atoi(argv[2]);

	int i = 0;
	int portNumbers[numClients];

	setupServer();

	for(i=0; i<numClients; i++)
	{
		portNumbers[i] = serverListen();
		
		printf("Received from port %d\n", portNumbers[i]);
	}

	for(i=0; i<numClients; i++)
	{
		if(i == numClients-1)
		{
			sprintf(msgBuffer, "%d", portNumbers[0]);
		}
		else
			sprintf(msgBuffer, "%d", portNumbers[i+1]);

		serverInfo.clientAddr.sin_port = htons(portNumbers[i]);
		serverSendMessage();
	}

	return 0;
	

}

void setupServer(void)
{
	if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{ 
		fprintf(stderr, "Cannot create socket\n"); 
		exit(0); 
	}

	memset((char *)&serverInfo.myAddr, 0, sizeof(serverInfo.myAddr)); 
	serverInfo.myAddr.sin_family = AF_INET; 
	serverInfo.myAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	serverInfo.myAddr.sin_port = htons(serverInfo.port);

	if (bind(sockFd, (struct sockaddr *)&serverInfo.myAddr, sizeof(serverInfo.myAddr)) < 0) 
	{ 
		fprintf(stderr, "Bind failed\n");
		exit(0); 
	}
}

void serverSendMessage(void)
{

	if (sendto(sockFd, msgBuffer, strlen(msgBuffer), 0, (struct sockaddr *)&serverInfo.clientAddr, sizeof(serverInfo.clientAddr)) < 0)
	{
		fprintf(stderr, "Sendto failed\n");
	}

}

int serverListen(void)
{
	printf("waiting on port %d\n", serverInfo.port); 
	bzero(msgBuffer, BUFF_SIZE);

	recvLen = recvfrom(sockFd, msgBuffer, BUFF_SIZE, 0, (struct sockaddr *)&serverInfo.clientAddr, &addrlen); 
	incomingPort = ntohs(serverInfo.clientAddr.sin_port);
	printf("received %d bytes from port %d\n", recvLen, incomingPort); 

	if (recvLen > 0) 
	{ 
		msgBuffer[recvLen] = 0; 
		printf("received message: %s", msgBuffer);
	}

	return incomingPort;
	
}
