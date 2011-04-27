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
	int sucuessorID; //'next' node
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
private:
	void tokenAquired();
	void handleCommands();
	void updateValues(set<int> addresses);
	void doComputations(stack<char *> commands);
	void grabLock(sem_t * lock);
	void postLock(sem_t * lock);
	void send(int nodeID, char * message);
	void write(int memAddress, int value, int nodeID);
	int read(int memAddress, int nodeID);
	void invalidate(int memAdress);
	void queueCommands(char * buf);
};
struct spawnNewRecieverInfo {void * node; int newConnectedSocket;};

#endif
