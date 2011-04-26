all: Nodes Utilities RunCoord RunDSM Coordinator

Nodes: node.o

Utilities : socket.o
	
node.o : node.cpp node.h socket.h socket.cpp
	g++ -c node.cpp

socket.o : socket.cpp socket.h
	g++ -c socket.cpp 

Coordinator : coordinator.cpp socket.h socket.cpp
	g++ -c coordinator.cpp

dsm : dsm.cpp dsm.h node.h node.cpp socket.h socket.cpp
	g++ -c dsm.cpp

RunCoord : Utilities Coordinator
	g++ -o coordinator coordinator.o node.o socket.o -lpthread

RunDSM : Nodes Utilities dsm
	g++ -o dsm dsm.o node.o socket.o -lpthread
clean : 
	rm  dsm coordinator node.o socket.o coordinator.o dsm.o


# DO NOT DELETE
