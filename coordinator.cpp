#include <iostream>
#include <vector>

int main(int argc, char * argv[])
{
	char * filename = argv[1];
	FILE * comfile = fopen(filename,"r");
	if(!comfile)
		perror("failed to open command file");
	char com[256];
	char tmp[256];
	while(fgets(com,sizeof(com),comfile) != NULL){
		strcpy(tmp,com);
		int sleeptime = atoi(strtok(com,"\n:"));
		sleep(sleeptime);
		int nodeID = atoi(strtok(NULL,"\n:"));

		//locate the 2nd colon for commandID+options
		int i,j;
		for(i = 0,j = 0; j < 2 && i < 256; i++){
			if(tmp[i] == ':')
				j++;
		}

		char * comAndOpt = tmp + i;
		//send comAndOpt to nodeID
		printf("%s\n",comAndOpt);
	}
	return 0;
}
