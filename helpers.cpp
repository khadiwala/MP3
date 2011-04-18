#include "helpers.h"
using namespace std;

/* parses membership.conf intro array of structs
   containing nodeid and port number*/
vector<nodeport*> parseMembers()
{
	int num;
	vector<nodeport*> members;
	ifstream mconf("membership.conf");
	if(!mconf)	
		perror("failed to open membership.conf");
	while (mconf >> num) {
		nodeport * s = new nodeport();
		s->node = num;
		mconf >> num;
		s->port = num;
		members.push_back(s);
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

/*int main(void)
{
	vector<int> v = parseMemMap();
	for(int i = 0; i < v.size(); i++)
		printf("%d\n",v[i]);
	return 0;
}*/
