#include "node.h"


Node::Node(int nodeID, int portNumber, vector<int>  memoryMap, map <int, int>portMap)
{
	DEBUGPRINT printf("Constructing node %i \n", nodeID);
  	this->nodeID = nodeID;
	workLock = new sem_t;
	lifeLock = new sem_t;
	strtokLock = new sem_t;	
	commandLock = new sem_t;
	localMemLock = new sem_t;
	sem_init(workLock, 0, 0);
	sem_init(lifeLock, 0, 0);
  	sem_init(strtokLock, 0, 1);
	sem_init(commandLock, 0, 1);
	sem_init(localMemLock, 0, 1);
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
	//also store succuessor
	for(int i = 0; i < memMap.size(); i++){
		if(memMap[i] == nodeID)
		{
			cacheEntry * entry = new cacheEntry();
			entry->value = 0;  //shared mem values initially 0
			nodeLocalMem[i] = entry; 
		}
	}

	//start trying to connect to other nodes! based on portMap
	map<int,int>::const_iterator end = portMap.end();
	successorID = -1;
	int last = -1;
	for(map<int,int>::const_iterator it = portMap.begin(); it!= end; it++)
	{
		//printf("key:%d; val:%d\n",it->first,it->second);
		if(it->first != nodeID) //dont connect to self
		{
			socketMap[it->first] = new_socket();
			printf("%d\n",it->first);
			connect(socketMap[it->first],it->second); //connect loops	
		}
		if(last == nodeID)	//set sucuessor
			successorID = it->first;
		last = it->first;
	}
	if(successorID == -1) //I believe this connects the ring in the case that the nodeID is last
		successorID = portMap.begin()->first;
}
Node::~Node()
{
	DEBUGPRINT printf("NODE:%d IS DEAD--------\n",this->nodeID);
	postLock(lifeLock);
	close(listeningSocket);
	listeningSocket = -1; ///>tells accepting thread to die
	pthread_t * oldThread;
	//cancel all threads
	while(!connectingThreads.empty())
	{
		oldThread = connectingThreads[connectingThreads.size()-1];
		connectingThreads.pop_back();
		pthread_cancel(*oldThread);
		delete oldThread;
	}
	//close all sockets
	map<int,int>::const_iterator end = socketMap.end();
	for(map<int,int>::const_iterator it = socketMap.begin(); it!= end; it++)
		close(it->second);
	//destroy semaphores
	sem_destroy(commandLock);
	sem_destroy(workLock);
	sem_destroy(strtokLock);
	sem_destroy(localMemLock);
	delete workLock;
	delete strtokLock;
	delete commandLock;
	delete localMemLock;
	sem_destroy(lifeLock);
	delete lifeLock;
}
	
void  Node::grabLock(sem_t * lock)
{
	DEBUGLOCK DEBUGPRINT printf("%i waiting for lock %p\n", nodeID, lock);
	while(sem_wait(lock) != 0)
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
	bool respond = false;
	bool acknowledge = false;
	string returnMessage = itoa(nodeID);
	grabLock(strtokLock);
	char * token = strtok(buf, ":");
	assert(token != NULL);
	int nodeID = atoi(token);
	if(nodeID == -1)
	{
		postLock(strtokLock);
		tokenAquired();
		return;
	}
	strtok(NULL, ":");
	int memAddr, value;
	while(token != NULL);
	{
		assert(strlen(token) == 1);
		memAddr = atoi(strtok(NULL, ":"));
		switch(token[0])
		{
			case 'R':
			case 'r': //read request
				respond = true;
				returnMessage += ":a:";
				returnMessage += memAddr;
				returnMessage += ':'; //a flags a response
				returnMessage += itoa(read(memAddr,nodeID));
				break;
			case 'A':
			case 'a': //acknowledge of read request
				acknowledge = true; 
			case 'W':
			case 'w': //write request
				value = atoi(strtok(NULL, ":"));	
				write(memAddr, value, nodeID);
				break;					
			case 'C': //queueCommands flag
			case 'c':
				postLock(strtokLock);
				queueCommands(buf);
				return;
			case 'i': //invalidate a memory address
			case 'I':
				invalidate(memAddr);
				break;
			default : 
				assert(false);
		}
		token = strtok(NULL, ":");
	}	
	postLock(strtokLock);
	if(respond)
		send(nodeID, (char *)returnMessage.c_str());	
	if(acknowledge)
		postLock(workLock);
}
void Node::tokenAquired()
{
	bool releaseFound;
	queue<char *> localQueue;
	grabLock(commandLock);
	if(! commands.empty())
	{
		assert(interpret(commands.front()) == AQUIRE);
		while(! commands.empty())//store queue locally / look for release command
		{
			localQueue.push(commands.front());
			commands.pop();
			if(interpret(localQueue.back()) == RELEASE)
			{
				releaseFound = true;
				break;
			}
		}
		if(releaseFound) //if release found in the queued instructions
		{
			postLock(commandLock);
			updateLocalMem(localQueue);
			handleCommands(localQueue);
			return;	
		}
		else //revert the queue back to the original
			commands = localQueue;
	}
	//if it gets here, the commands were not handled	
	postLock(commandLock);
	releaseToken();
}
void Node::updateLocalMem(queue<char *> commands)
{
	char * command, tempCommand[BUFFER_SIZE], char * token;
	//iterate through the queue, reads through commands and updates memory
	grabLock(strtokLock);
	for(int i = 0; i < queue.size(); i++)
	{
		command = queue.front();
		queue.pop();
		queue.push(command);
		Command interp = interpret(command);
		if(interp == ADD || interp == PRINT)
		{
			strcpy(tempCommad, command);
			token = strtok(tempCommand, ":"); //ignore first command number
			token = strtok(NULL, ":");
			int memAddr = atoi(token); 
			while(token != NULL)
			{
				token = strtok(NULL, ":");
				if(token != NULL || interp == PRINT) //add should not interpret last entry as address
					updateIfNeeded(memAddr);
				if(token != NULL)
					memAddr = atoi(token);
			}
		}
	}	
	postLock(strtokLock);
}
void Node::updateIfNeeded(int memAddr)
{}
void Node::handleCommands(queue<char *> commands)
{}
void Node::releaseToken()
{
	send(successorID, "-1");
}
void Node::queueCommands(char * buf)
{
	grabLock(commandLock);
	char command[BUFFER_SIZE];
	int numColons = 0, i;
	for(i = 0; i < BUFFER_SIZE; i++) //getting rid of meta data needed for handle
		if(buf[i] == ':' && numColons++ == 2)
			break;
	strcpy(command, buf+i+1); 
	commands.push(command);
	postLock(commandLock);
}
Node::Command Node::interpret(char * command)
{
	char commandNumber[2];
	strncpy(commandNumber, command, sizeof(char));
	commandNumber[2] = 0;
	int number = atoi(commandNumber);
	return (Command)number;
}
void Node::send(int nodeID, char * message)
{
	s_send(socketMap[nodeID], message);	
}
void Node::write(int memAddress, int value, int nodeID)
{
	grabLock(localMemLock);
	cacheEntry * entry = nodeLocalMem[memAddress];
	char invalidate_msg[BUFFER_SIZE];
	//set invalidate_msg

	while(entry->readers.size() > 0) 
	{
		int rdr = entry->readers.back();
		entry->readers.pop_back();
		if(rdr != nodeID)	// don't want to invaliate writer's val
		{
			send(rdr,invalidate_msg);
		}
	}	
	entry->readers.push_back(nodeID); //the writer has a cache copy
	postLock(localMemLock);
}
int Node::read(int memAddress,int nodeID)
{
	grabLock(localMemLock);
	cacheEntry * entry = nodeLocalMem[memAddress];
	entry->readers.push_back(nodeID);//mark this nodeID as a reader
	int ret = entry->value;
	postLock(localMemLock);
	return ret;
}

void Node::invalidate(int memAdress)
{
	grabLock(localMemLock);
	cacheEntry * entry = nodeLocalMem[memAdress];
	entry->readers.clear();
	delete entry;
	nodeLocalMem.erase(memAdress);
	postLock(localMemLock);
}
void * acceptConnections(void * nodeClass)
{
	Node * node = (Node *)nodeClass;
	spawnNewRecieverInfo info;
	info.node = node;
	///start accepting connections
	pthread_t * newThread;
	int connectingSocket;
	while(node->listeningSocket != -1)
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

