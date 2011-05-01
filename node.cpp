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
			DEBUGPRINT printf("node %d waiting for node %d\n",nodeID,it->first);
			connect(socketMap[it->first],it->second); //connect loops	
			DEBUGPRINT printf("node %d connected to node %d\n",nodeID,it->first);
		}
		if(last == nodeID)	//set sucuessor
			successorID = it->first;
		last = it->first;
	}
	if(successorID == -1) //I believe this connects the ring in the case that the nodeID is last
		successorID = portMap.begin()->first;

	cout<<nodeID<<" SUCCESSOR ID "<<successorID<<endl;
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
	DEBUGPRINT printf("Node: %d recieved command %s\n",nodeID,buf);
	bool respond = false;
	bool acknowledge = false;
	char returnMessage[BUFFER_SIZE];
	set<int> ackWriteNodes;
	strcpy(returnMessage, itoa(nodeID));
	grabLock(strtokLock);
	char copy[BUFFER_SIZE];
	strcpy(copy, buf);
	char * token = strtok(copy, ":");
	int commandNodeID = atoi(token);
	if(commandNodeID == -1)
	{
		postLock(strtokLock);
		tokenAquired();
		return;
	}
	token = strtok(NULL, ":");
	int memAddr, value;
	while(token != NULL)
	{
		char * tempMemAddr = strtok(NULL, ":");
		if(tempMemAddr != NULL)
			memAddr = atoi(tempMemAddr);

		switch(token[0])
		{
			case 'R':
			case 'r': //read request
				DEBUGPRINT printf("node %i read request\n", nodeID);
				respond = true;
				strcat(returnMessage,":a:");
				strcat(returnMessage,itoa(memAddr));
				strcat(returnMessage, ":"); //a flags a response
				strcat(returnMessage,itoa(read(memAddr,commandNodeID)));
				break;
			case 'k':
			case 'K':
				//acknowledge of write
				DEBUGPRINT printf("node %i ackonowledge of write\n", nodeID);
				acknowledge = true;
				break;
			case 'A':
			case 'a': //acknowledge of read request
				DEBUGPRINT printf("node %i acknowledge of read\n", nodeID);
				acknowledge = true; 
			case 'W':
			case 'w': //write request
				DEBUGPRINT printf("node %i write request\n", nodeID);
				value = atoi(strtok(NULL, ":"));	
				write(memAddr, value, commandNodeID);
				if(token[0] != 'a' && token[0] != 'A')
					ackWriteNodes.insert(commandNodeID);
				break;					
			case 'C': //queueCommands flag
			case 'c':
				DEBUGPRINT printf("node %i coordinator request\n", nodeID);	
				postLock(strtokLock);
				queueCommands(buf);
				return;
			case 'i': //invalidate a memory address
			case 'I':
				DEBUGPRINT printf("node %i invalide request\n", nodeID);
				invalidate(memAddr);
				break;
			default : 
				assert(false);
		}
		token = strtok(NULL, ":");
	}	
	postLock(strtokLock);
	if(respond)
		send(commandNodeID, returnMessage);	
	if(acknowledge)
		postLock(workLock);
	
	//send acknowledge of write to each node that wrote
	set<int>::const_iterator ackEnd = ackWriteNodes.end();
	for(set<int>::const_iterator it = ackWriteNodes.begin(); it!= ackEnd; it++)
	{
		DEBUGPRINT printf("%i acknowledge write to node %i\n", this->nodeID, *it);
		send(*it, "0:k:");	
	}
}
void Node::tokenAquired()
{
	sleep(1);//prevents crazy fast token passing
	DEBUGPRINT printf("node %i aquiring token\n\tqueue size: %d\n", nodeID,commands.size());
	bool releaseFound;
	queue<char *> localQueue;
	grabLock(commandLock);
	if(! commands.empty())
	{
		//assert(interpret(commands.front()) == AQUIRE);
		while(! commands.empty())//store queue locally / look for release command
		{
			localQueue.push(commands.front());
			//DEBUGPRINT printf("command's size: %d\n",commands.size());
			//DEBUGPRINT printf("%i command -%s\n", nodeID, localQueue.back()); 
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
			releaseToken();
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
	char * command, tempCommand[BUFFER_SIZE], * token;
	set<int> memToUpdate;

	//iterate through the queue, reads through commands and finds all memory address that we're going to use
	grabLock(strtokLock);
	for(int i = 0; i < commands.size(); i++)
	{
		command = commands.front();
		commands.pop();
		commands.push(command);
		Command interp = interpret(command);
		if(interp == ADD || interp == PRINT)
		{
			strcpy(tempCommand, command);
			strtok(tempCommand, ":"); //ignore first command number
			if(interp == ADD)
				token = strtok(NULL, ":"); //ignore first memAddr, we write to this	
			token = strtok(NULL, ":");
			int memAddr = atoi(token); //first address
			while(token != NULL)
			{
				token = strtok(NULL, ":");
				if(token != NULL || interp == PRINT) //add should not interpret last entry as address, print should
					memToUpdate.insert(memAddr);
			}
		}
	}	
	postLock(strtokLock);
	
	//update memory
	map<int , char *> nodeRequest;
	set<int>::const_iterator memToUpdateEnd = memToUpdate.end();
	for(set<int>::const_iterator it = memToUpdate.begin(); it!= memToUpdateEnd; it++)
	{
		if(nodeLocalMem.count(*it) == 0) //if we don't have a local copy already
		{
			int node = memMap[*it]; //node where the byte is stored at
			if(nodeRequest.count(node) == 0)//if this is the first request to this node build message
			{
				//char * newEntry = nodeRequest[node];
				char * newEntry = new char[BUFFER_SIZE];
				strcpy(newEntry, itoa(nodeID));
				nodeRequest[node] = newEntry;
			}
			char * message = nodeRequest[node];
			strcat(message, ":r:");
			strcat(message, itoa(*it));
		}
	}
	
	memToUpdate.clear();
	//send requests
	int sentMessages = 0;
	map<int, char *>::const_iterator nodeRequestEnd = nodeRequest.end();
	for(map<int, char *>::const_iterator it = nodeRequest.begin(); it != nodeRequestEnd; it++) 
	{
		sentMessages++;
		send(it->first, it->second);
		printf("node %d sent %s to %d\n",this->nodeID,it->second,it->first);
		delete it->second;
	}
	nodeRequest.clear();
DEBUGPRINT printf("%i waiting on %d acknowledges\n",this->nodeID, sentMessages);
	
	//wait for all acknowledges to finish before returning
	for(int i = 0; i < sentMessages; i++) 	{
		DEBUGPRINT printf("waiting on ack number %d\n",i);
		grabLock(workLock);
	}
}

void Node::handleCommands(queue<char *> commands)
{
	char * command;
	char * token;
	map<int , char *> nodeToWrite;
	grabLock(strtokLock);
	grabLock(localMemLock);
	DEBUGPRINT printf("node %d's commands queue has size %d\n",nodeID,commands.size());
	while(commands.size() != 0)
	{
		command = commands.front();
		DEBUGPRINT printf("node %d handling command %s\n",this->nodeID,command);
		commands.pop();
		switch(interpret(command))
		{
			case ADD:
			{
				DEBUGPRINT printf("\n%i adding: \n", this->nodeID);
				vector<int> memAddr;
				token = strtok(command, ":"); ///>ignore type
				token = strtok(NULL, ":"); ///> grab write address
				int writeAddr = atoi(token);
				token = strtok(NULL, ":"); ///> grab first byte address 
				int value = atoi(token);
				//stores all values to the vector except the last
				while(token != NULL)
				{
					memAddr.push_back(value);
					value = atoi(token);
					token = strtok(NULL, ":");
				}
				int sum = 0;
				
				//add up all the stored bytes
				for(int i = 0; i < memAddr.size(); i++)
				{
					DEBUGPRINT printf("%i value %i\n", memAddr[i], nodeLocalMem[memAddr[i]]->value);
					sum += nodeLocalMem[memAddr[i]]->value;
				}
				sum += value; ///> add the last int
				printf("total sum for addr %i = %i\n", writeAddr, sum);
				// add command string on to the nodeToWrite if it doesn't exist
				int writeNodeID = memMap[writeAddr];
				if(writeNodeID != this->nodeID)
				{ 
					if(nodeToWrite.count(writeNodeID) == 0)
					{
						DEBUGPRINT printf("%i creating new write message for %i\n", this->nodeID, writeNodeID);
						char * newEntry = new char[BUFFER_SIZE];
						nodeToWrite[writeNodeID] = newEntry;
						strcpy(newEntry, itoa(nodeID));
					}
					char * messageToSend = nodeToWrite[writeNodeID];
					strcat(messageToSend, ":w:");		
					strcat(messageToSend, itoa(writeAddr)); //address
					strcat(messageToSend, ":"); 
					strcat(messageToSend, itoa(sum)); //value
					DEBUGPRINT printf("%i new send message to %i: %s\n", this->nodeID, writeNodeID, messageToSend);
				}
			}
				break;
			case PRINT:
			{
				int garbage = atoi(strtok(command,":"));
				int memloc = atoi(strtok(NULL,":"));
				cout<<"Memory location "<<memloc<<" contains "<<nodeLocalMem[memloc]->value
					<<" stored at "<<memMap[memloc]<<endl;
			}
				break;
		}
		delete command;
	}
	int messagesSent = 0;
	map<int, char *>::const_iterator writeEnd = nodeToWrite.end();
	for(map<int, char *>::const_iterator it = nodeToWrite.begin(); it != writeEnd; it++) 
	{
		messagesSent++;
		send(it->first, it->second);
		delete it->second;
	}
	nodeToWrite.clear();
	postLock(strtokLock);
	postLock(localMemLock);
	//wait for acknoledges for the sends
	for(int i = 0; i < messagesSent; i++)
		grabLock(workLock);
	DEBUGPRINT printf("node %d finished handeling commands\n",nodeID);
}
void Node::releaseToken()
{
	send(successorID, "-1");
}
void Node::queueCommands(char * buf)
{
	grabLock(commandLock);
	char * command = new char[BUFFER_SIZE];
	int j, i;
	for(i = 0,j = 0; j < 2 && i < BUFFER_SIZE; i++){
		if(buf[i] == ':')
			j++;
	}

	strcpy(command, buf+i); 
	commands.push(command);
	DEBUGPRINT printf("%i queueing - %s. now has size %d\n\n", nodeID, command,commands.size());
	postLock(commandLock);
}
Node::Command Node::interpret(char * command)
{
	char commandNumber[2];
	strncpy(commandNumber, command, sizeof(char));
	commandNumber[2] = 0;
	int number = atoi(commandNumber);
	return (Command) (number - 1);
}
void Node::send(int nodeID, char * message)
{
	DEBUGPRINT printf("%i sending %s to %i\n", this->nodeID, message, nodeID);
	s_send(socketMap[nodeID], message);	
}
void Node::write(int memAddress, int value, int nodeID)
{
	grabLock(localMemLock);
	cacheEntry * entry;
	if(nodeLocalMem.count(memAddress) == 0)//create new entry if nesscry
	{
		entry = new cacheEntry();
		nodeLocalMem[memAddress] = entry;
	}
	entry = nodeLocalMem[memAddress];
	entry->value = value;
	if(memMap[memAddress] == this->nodeID) //if this node is the owner, send invalidates
	{
		char invalidate_msg[BUFFER_SIZE];
		//set invalidate_msg
		strcpy(invalidate_msg, itoa(Node::nodeID));
		strcat(invalidate_msg, ":i:");
		strcat(invalidate_msg, itoa(memAddress));
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
	}
	postLock(localMemLock);
}
int Node::read(int memAddress,int nodeID)
{
	grabLock(localMemLock);
	cacheEntry * entry = nodeLocalMem[memAddress];
	entry->readers.push_back(nodeID);//mark nodeID as a reader
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

