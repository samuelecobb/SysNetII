/*
 * tokRingClient.h
 *
 *  Created on: Apr 21, 2018
 *      Author: samuel cobb
 */

#ifndef TOKRINGCLIENT_H_
#define TOKRINGCLIENT_H_

typedef struct NodeInfo{
	int serverPort; //port which program acts as server (currNode-1)
	int peerPort; //port which program acts as client (currNode)
	struct sockaddr_in 	servAddr; //same as with ports
	struct sockaddr_in  myAddr; //same as with ports
}NodeInfo;

typedef enum OptionList{
	writeOp,
	readOp,
	listOp,
	exitOp
}OptionList;


/*Client Functions*/
void joinTokenRing(void);
void clientSendMessage(void);
void clientListen(void);
void setupNode(void);
void connectToServer(void);

/*Server Functions*/
void setupServer(void);
void waitForConnection(void);
void serverSendMessage(void);
void serverListen(void);

/*Misc Functions*/
void* tokenHandler(void*);
void printMenu();
OptionList translateOption(char);
void bbWrite();
void bbRead();
void bbList();
void bbExit();

#endif /* TOKRINGCLIENT_H_ */
