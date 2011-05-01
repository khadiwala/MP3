#ifndef _NODE_H_
#define _NODE_H_

#include <stack>
#include <set>
#include <string>
#include <map>
#include <assert.h>
#include <vector>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include "socket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DSM_SIZE 100
#define BUFFER_SIZE 256
#define DEBUGPRINT if(false)
#define DEBUGLOCK if(false)
using namespace std;


/////////////////////////////////////////////////
// creates a thread to handle a command from 
// a socket
////////////////////////////////////////////////
void * handleThread(void * handleInfo);

//////////////////////////////////////////////////
// Creates a thread to listen to a connecting socket
// and take apporpriate action
/////////////////////////////////////////////////
void * spawnNewReciever(void * NodeClass);


/////////////////////////////////////////////////
//  this is used in another thread to accept connections
//  and spawn additional threads
//////////////////////////////////////////////////
void * acceptConnections(void * NodeClass);

struct cacheEntry {int value; vector<int> readers;};

class Node
{
//protected:
public:
	
	Node(int nodeID, int portNumber,vector<int> memoryMap, map<int, int>portMap);
	~Node();
	int nodeID; ///> relative node id to the system
	int successorID; //'next' node
	volatile int listeningSocket; 
	vector<int> memMap; // shows at which node a byte is located (nodeID)
	map<int, cacheEntry *> nodeLocalMem; // maps locations at this node to values
	map<int, int> socketMap; // shows where a node id located (socket)
	sem_t * commandLock;
	sem_t * workLock; ///> gets unlocked when a node replies to this node with requested data
	sem_t * lifeLock; ///>lock becomes unlocked in the destructor
	sem_t * strtokLock; ///>used in conjunction with strtok because it is not thread safe
	sem_t * localMemLock;///>for reading/writing node's local memory
   	vector<pthread_t*> connectingThreads; ///>holds all threads the node spawns
	queue<char *> commands; ///> holds all the commands from the queueCommands
	
	void handle(char * buf); //handle requests from other nodes
	enum Command {AQUIRE, RELEASE, ADD, PRINT};
	void grabLock(sem_t * lock);
	void postLock(sem_t * lock);

private:
	/**
	*  interprets the command number of the command and returns the Command 
	*/
	Command interpret(char * command);
	
	/**
	* function is called when this node aquires the token, it should c
	* check to see if work is needed to be done and do it
	*/
	void tokenAquired();
	
	/**
	* sends a message to the next node giving them the token
	*/ 
	void releaseToken();
	
	/**
	* before doing any commands, we update all memory addresses
	*/
	void updateLocalMem(queue<char *> commands);
	
	/**
	* called when this node has the token
	*/
	void handleCommands(queue<char *> commands);
	
	void send(int nodeID, char * message);
	
	/**
	* nodeID is requesting a write of value to memAddress
	*/
	void write(int memAddress, int value, int nodeID);
	
	/**
	* nodeID is requesting a read of memAddr
	*/
	int read(int memAddress, int nodeID);
	
	/**
	* called when a node sends an invalidate memory command, this node should remove that memAddr from it's cache
	*/ 
	void invalidate(int memAdress);
	
	/**
	* called when coordinator sends a command to this node
	*/
	void queueCommands(char * buf);
};
struct spawnNewRecieverInfo {void * node; int newConnectedSocket;};
struct HandleInfo { Node * node; char * buf; };
#endif
