#include "socket.h"

//returns socket fd
int new_socket()
{
    addrinfo hints;
    addrinfo * sinfo;
    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sinfo = new addrinfo();
    if(getaddrinfo(NULL,"3748",&hints,&sinfo) != 0)
        perror("failed to setup struct in new_socket");
    int sockfd = socket(sinfo->ai_family, sinfo->ai_socktype,
                 sinfo->ai_protocol);

    //dont hang on port after execution
    int iSetOption = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,
              (char*)&iSetOption,sizeof(iSetOption));

    free(sinfo);
    return sockfd;
} 

//binds socket fd to a port number (string)
void bind(int sfd,int portNum)
{
    char * port = itoa(portNum);
    addrinfo * sinfo;
    addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sinfo = new addrinfo();
    if(getaddrinfo(NULL,port,&hints,&sinfo) != 0)
        perror("failed to setup struct in new_socket");
    if(bind(sfd,sinfo->ai_addr,sinfo->ai_addrlen) == -1)
        perror("failed to bind socket");

    delete port;
}

void listen(int sfd)
{
    if(listen(sfd,BACKLOG) == -1)
        perror("listen failed");
}

/* accept a connection on a listening socket
 * returns a new socket id to read and write on
 */
int accept(int sfd)
{
    int ret = accept(sfd,NULL,NULL);
    if(ret == -1)
        perror("accept failed");
    return ret;
}

/* connect socket (to a listeing socket on port port) */
void connect(int sfd, int portNum)
{
    char * port = itoa(portNum);
    addrinfo * sinfo;
    addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    sinfo = new addrinfo();
    if(getaddrinfo(NULL,port,&hints,&sinfo) != 0)
        perror("failed to setup struct in new_socket");
    while(connect(sfd,sinfo->ai_addr,sinfo->ai_addrlen) == -1)
    {
       // printf("sock:%d,pn:%d\n",sfd,portNum);
       // perror("failed to connect");
        sleep(1);
    }
    delete port;
} 

/* reads sizeOfBuf characters from socket into buffer 
 * buf must be already allocated 
 * returns false if socket is dead*/
bool s_recv(int sfd, char * buf, int sizeOfBuf)
{
    int status = recv(sfd,buf,sizeOfBuf,0);
    if(status == -1) {
        perror("error recieving from socket");
        return false;
    }
    else if(status == 0)
        return false;
    else
        return true;
    
	//for(int i = 0; i < 5; i ++ )
    //{
	//	if(recv(sfd,buf, sizeOfBuf,0) != -1)
	//		return true;
    //    else
    //        perror("error recieving from socket");
    //}
	//cout<<"socket has died\n";
	//return false;
} 

/* writes strlen(buf) characters from buf into socket*/
void s_send(int sfd, char* buf)
{
    if(send(sfd,buf,strlen(buf),0) == -1)
        perror("error writing to socket");
    if(send(sfd,"+",strlen("+"),0) == -1)
        perror("error writing . to socket");
}


char * itoa(int a)
{
	char * digits = new char[6];	
	sprintf(digits,"%d",a);
	return digits;
}

