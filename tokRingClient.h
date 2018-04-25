/*
 * tokRingClient.h
 *
 *  Created on: Apr 21, 2018
 *      Author: samuel cobb
 */

#ifndef TOKRINGCLIENT_H_
#define TOKRINGCLIENT_H_

typedef struct NodeInfo{
	int nextPort; //port which program acts as server (currNode-1)
	int myPort; //port which program acts as client (currNode)
	struct sockaddr_in 	nextAddr; //same as with ports
	struct sockaddr_in  myAddr; 
	struct sockaddr_in 	prevAddr; //same as with ports
}NodeInfo;

typedef enum OptionList{
	writeOp,
	readOp,
	listOp,
	exitOp
}OptionList;



// joinTokenRing(void)
// 	-Used to call all the necessary functions to insert node into tokenRing
void joinTokenRing(void);


// clientSendMessage(void)
// 	-Used to send message to the main server so the server can get a list of node addresses
void clientSendMessage(void);


// clientListen(void)
// 	-Called to listen for response from server. Receives address for next node in ring
void clientListen(void);


// setupNode(void)
// 	-Creates a socket for the current node
void setupNode(void);


// connectToServer(void)
// 	-Establishes communication between this node and the new next node
void connectToServer(void);


/*Server Functions*/
void setupServer(void);


void* tokenHandler(void*);
void printMenu();
OptionList translateOption(char);
void bbWrite(void);
void bbRead(void);
void bbList(void);
void bbExit(void);
int sendMessage(char*);
int receiveMessage(void);

#endif /* TOKRINGCLIENT_H_ */
