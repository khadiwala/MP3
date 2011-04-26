#ifndef _NODE_H_
#define _NODE_H_

#include <string>
#include <map>
#include <vector>
#include <pthread.h>
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

enum instance { NODE, DEAD };

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

class Node
{
//protected:
public:
	
	Node(int nodeID, int portNumber,vector<int> memoryMap, map<int, int>portMap);
	~Node();
		
	int nodeID; ///> relative node id to the system
	int listeningSocket; 
	vector<int> memMap; // shows at which node a byte is located (nodeID)
	map<int, int> nodeLocalMem; // maps locations at this node to values
	map<int, int> socketMap; // shows where a node id located (socket)
	volatile instance status;
	sem_t * strtokLock;
	vector<pthread_t*> connectingThreads;
	
	void handle(char * buf);
	///These should be called before changing class data
	void grabLock(sem_t * lock);
	void postLock(sem_t * lock);
	
};
struct spawnNewRecieverInfo {void * node; int newConnectedSocket;};

#endif
