#include <iostream>
#include <vector>
#include "node.h"
#include <time.h>
#include <unistd.h>
#include <map>
#include "socket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>

/* parses membership.conf into map from nodeid to port num
   containing nodeid and port number*/
map<int,int> parseMembers()
{
	int num;
	map<int,int> members;
	ifstream mconf("membership.conf");
	if(!mconf)	
		perror("failed to open membership.conf");
	int nodeid,port;
	while (mconf >> nodeid) {
		mconf >> port;
		members[nodeid] = port;
	}	
	return members;
}

int main(int argc, char * argv[])
{
	if(argc < 2)
	{
		cout<<"please enter a file name\n";
		exit(-1);
	}
	char * filename = argv[1];
	FILE * comfile = fopen(filename,"r");
	if(!comfile)
	{
		perror("failed to open command file");
		exit(-1);
	}

	map<int,int> ports = parseMembers();
	map<int,int> nodeToSocket;
	
	map<int, int>::const_iterator endports = ports.end();
	for(map<int, int>::const_iterator it = ports.begin(); it != endports; it++) 
	{
		nodeToSocket[it->first] = new_socket();
		connect(nodeToSocket[it->first],it->second);
	}
	ports.clear();
	s_send(nodeToSocket[0],"-1");
	
	char com[BUFFER_SIZE];
	char tmp[BUFFER_SIZE];
	while(fgets(com, BUFFER_SIZE,comfile) != NULL){
		strcpy(tmp,com);
		unsigned int sleeptime = atoi(strtok(com,"\n:"));
		sleeptime = sleeptime * 1000;
		usleep(sleeptime);
		int nodeID = atoi(strtok(NULL,"\n:"));

		//locate the 2nd colon for commandID options
		int i,j;
		for(i = 0,j = 0; j < 2 && i < BUFFER_SIZE; i++){
			if(tmp[i] == ':')
				j++;
		}

		char * comAndOpt = tmp + i;
		char moretmp[BUFFER_SIZE];
		strcpy(moretmp,"0:C:");
		strcat(moretmp,strtok(comAndOpt,"\n")); //strtok strips new line from command
		//send comAndOpt to nodeID
		printf("sending %s to %d\n",moretmp,nodeID);
		s_send(nodeToSocket[nodeID],moretmp);
	}
	return 0;
}

