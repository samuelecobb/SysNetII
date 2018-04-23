#ifndef TOKRINGSERVER_H
#define TOKRINGSERVER_H

#define BUFF_SIZE 2048 

typedef enum Operation{
	writeOp,
	readOp,
	listOp,
	exitOp
}Operation;

typedef struct ServerInfo{
	int port; //port which the server listens
	struct sockaddr_in 	myAddr; //server address
	struct sockaddr_in  clientAddr; //client address
}ServerInfo;

void setupServer(void);
void waitForConnection(void);
void serverSendMessage(void);
void serverListen(void);

#endif
