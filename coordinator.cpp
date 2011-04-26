#include <iostream>
#include <vector>
#include "node.h"

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
	char com[BUFFER_SIZE];
	char tmp[BUFFER_SIZE];
	while(fgets(com, BUFFER_SIZE,comfile) != NULL){
		strcpy(tmp,com);
		int sleeptime = atoi(strtok(com,"\n:"));
		sleep(sleeptime);
		int nodeID = atoi(strtok(NULL,"\n:"));

		//locate the 2nd colon for commandID options
		int i,j;
		for(i = 0,j = 0; j < 2 && i < BUFFER_SIZE; i++){
			if(tmp[i] == ':')
				j++;
		}

		char * comAndOpt = tmp + i;
		//send comAndOpt to nodeID
		printf("%s\n",comAndOpt);
	}
	return 0;
}
