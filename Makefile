all: Nodes Utilities Run 

Nodes: node.o

Utilities : socket.o helpers.o
	
node.o : node.cpp node.h socket.h
	g++ -c node.cpp

socket.o : socket.cpp socket.h
	g++ -c socket.cpp 

helpers.o : helpers.cpp helpers.h
	g++ -c helpers.cpp

Run : Nodes Utilities
	g++ -o dsm node.o helpers.o socket.o -lpthread -lm
clean : 
	rm  dsm node.o socket.o helpers.o 


# DO NOT DELETE
