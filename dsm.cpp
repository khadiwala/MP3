#include "dsm.h"
using namespace std;

int main(int argc, char* argv[]){
	if(argc == 1){
		printf("Usage: ./dsm <nodeid>\n");
		exit(1);
	}
	int nodeID = atoi(argv[1]);
	map<int,int> portMap = parseMembers();
	if(portMap.count(nodeID) < 0){
		printf("No port number for nodeID argument in membership.conf\n");
		exit(1);
	}
	int nodePort = portMap[nodeID];
	vector<int> memMap = parseMemMap();
	Node * n = new Node(nodeID,nodePort,memMap,portMap);
	n->grabLock(n->lifeLock);
	cout<<"dead\n"; 
	
}

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

/*parses memory_map.conf into an vector
  corresponding to node locations of mem*/
vector<int> parseMemMap()
{
	int num;
	vector<int> map;
	ifstream mapconf("memory_map.conf");
	if(!mapconf)
		perror("failed to open memory_map.conf");
	while (mapconf >> num) {
		mapconf >> num; //skips the index
		map.push_back(num);
	}
	return map;
}
