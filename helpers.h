#include <iostream>
#include <fstream>
#include <vector>

struct nodeport{int node; int port;};
std::vector<nodeport*> parseMembers();
std::vector<int> parseMemMap();
