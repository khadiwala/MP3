#include "node.h"


Node::Node(int nodeID, int portNumber, vector<int>  memoryMap, map <int, int>portMap)
{
	DEBUGPRINT printf("Constructing node %i \n", nodeID);
  	this->nodeID = nodeID;
	status = NODE;
	strtokLock = new sem_t;
  	sem_init(strtokLock, 0, 1);
	memMap = memoryMap;
	
	/// set up socket
	listeningSocket = new_socket();
	bind(listeningSocket, portNumber);
	listen(listeningSocket);

	//spawn an acceptor process
	pthread_t * acceptor = new pthread_t;
	connectingThreads.push_back(acceptor);
	pthread_create(acceptor, NULL, 
			acceptConnections, this);

	//step through memmap, if a location is 
	//at this nodeid add to nodeLocalMem map
	for(int i = 0; i < memMap.size(); i++){
		if(memMap[i] == nodeID)
			nodeLocalMem[i] = 0; //shared mem values initially 0
	}

	//start trying to connect to other nodes! based on portMap
	map<int,int>::const_iterator end = portMap.end();
	for(map<int,int>::const_iterator it = portMap.begin(); it!= end; it++)
	{
		//printf("key:%d; val:%d\n",it->first,it->second);
		if(it->first != nodeID) //dont connect to self
		{
			socketMap[it->first] = new_socket();
			printf("%d\n",it->first);
			connect(socketMap[it->first],it->second); //connect loops	
		}
	}
}
Node::~Node()
{
    DEBUGPRINT printf("NODE:%d IS DEAD--------\n",this->nodeID);
    status = DEAD;
    close(listeningSocket);
    pthread_t * oldThread;
    sem_destroy(strtokLock);
    while(!connectingThreads.empty())
    {
    	oldThread = connectingThreads[connectingThreads.size()-1];
    	connectingThreads.pop_back();
    	pthread_cancel(*oldThread);
    	delete oldThread;
    }
}
	
void  Node::grabLock(sem_t * lock)
{
	DEBUGLOCK DEBUGPRINT printf("%i waiting for lock %p\n", nodeID, lock);
	while(sem_wait(lock) != -0)
		DEBUGPRINT cout<<"waiting on the lock failed, trying again\n";
}
void Node::postLock(sem_t * lock)
{
	DEBUGLOCK DEBUGPRINT printf("%i posting lock %p\n", nodeID, lock);
	while(sem_post(lock) != 0)
		DEBUGPRINT cout<<"posting to the lock failed, trying again\n";
}
void Node::handle(char * buf)
{
}
void * acceptConnections(void * nodeClass)
{
	Node * node = (Node *)nodeClass;
	spawnNewRecieverInfo info;
	info.node = node;
	///start accepting connections
	pthread_t * newThread;
	int connectingSocket;
	while(node->status != DEAD)
	{
		connectingSocket = accept(node->listeningSocket);
		if(connectingSocket != -1)
		{
			///Creates new thread to listen on this connection
			newThread = new pthread_t;
			info.newConnectedSocket = connectingSocket;
			node->connectingThreads.push_back(newThread);
			pthread_create(newThread, NULL, 
						spawnNewReciever, &info);
		}
	}
	return NULL;	
}

void * spawnNewReciever(void * information)
{
	spawnNewRecieverInfo info =*((spawnNewRecieverInfo*)information);
	Node * node = (Node*)info.node;
	int connectedSocket = info.newConnectedSocket;
	char * c = new char[2];
	char * buf = new char[BUFFER_SIZE];
	buf[0] = 0;
	int i = 0;
	while(s_recv(connectedSocket, c, 1))	
	{
        	c[1] = 0;
        	if(strcmp(c,"+") == 0)
        	{
    			node->handle(buf);
        		strcpy(buf,"");
        	}
        	else
            		strcat(buf,c);
	}
	close(connectedSocket);
	delete c;
	delete buf;
  return NULL;
}

