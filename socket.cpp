#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h> 

/*********************   CLASS DECLARATION    *********************/

class ReadSocket {

	private:
		int sockfd, portno;
		socklen_t clilen,n;

		char * ip;

		bool socketBusy;
		char buffer[256];

		struct sockaddr_in serv_addr;
		struct hostent *server;

		void error(const char * message);

	public:
		ReadSocket(void); //constructor
		~ReadSocket(void); //destructor

		void connect(char * ip);
		void write(const char * command);
		char * read();
		bool isConnected();
		void close();

};

/*********************   FUNCTIONS    *********************/


/** Constructor */
ReadSocket::ReadSocket(void) {

	this->socketBusy = false;
	this->portno = 3000;

}

/** Destructor */
ReadSocket::~ReadSocket(void) {

}

void ReadSocket::connect(char * ip){

	if(this->socketBusy==true)
	{
		std::cout << "(" << this->ip << ") - Socket already open for this host" << std::endl;
		return;
	}

	this->ip = ip;

	std::cout << "(" << this->ip << ") - Trying to connect to slave..." << std::endl;

	this->server = ::gethostbyname(this->ip);

	int iSetOption = 1;

	this->sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,sizeof(iSetOption));

	if (this->sockfd < 0) 
		std::cout << "(" << this->ip << ") - Could not open listen socket" << std::endl;

	bzero((char *) &this->serv_addr, sizeof(this->serv_addr));

	this->serv_addr.sin_family = AF_INET;
	bcopy((char *)this->server->h_addr, (char *)&this->serv_addr.sin_addr.s_addr,this->server->h_length);
	this->serv_addr.sin_port = htons(portno);

	if (::connect(this->sockfd,(struct sockaddr *) &this->serv_addr,sizeof(this->serv_addr)) < 0)
		std::cout << "(" << this->ip << ") - Could not connect to slave" << std::endl;
	else
	{
		this->socketBusy = true;
		std::cout << "(" << this->ip << ") - Connected to slave successfuly" << std::endl;
	}

	fd_set fdset;
	struct timeval tv;
	FD_ZERO(&fdset);
	FD_SET(this->sockfd, &fdset);
	tv.tv_sec = 5;
	tv.tv_usec = 500000;

	if (select(this->sockfd + 1, NULL, &fdset, NULL, &tv) == 1)
	{
		setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
		setsockopt(this->sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(struct timeval));
	}

}

void ReadSocket::write(const char * command){

	this->n = ::write(this->sockfd,command,strlen(command));

	if (this->n < 0) 
		std::cout << "(" << this->ip << ") - ERROR writing to socket on slave" << std::endl;

}

char * ReadSocket::read(){

	bzero(this->buffer,256);

	memset(this->buffer, 0, sizeof(this->buffer));

	this->n = ::read(this->sockfd,this->buffer,255);
	
	if (this->n < 0) 
		std::cout << "(" << this->ip << ") - ERROR reading from socket" << std::endl;

	int i=0;

	for(i=0;i<=255;i++)
	{
		if((int)this->buffer[i]==13)
		{
			this->buffer[i] = '\0';
		}
	}

	char * output = (char*) malloc(256*sizeof(char));
	strcpy(output, this->buffer);

	return output;

}

void ReadSocket::close(){

	::close(this->sockfd);

	std::cout << "(" << this->ip << ") - Connection closed" << std::endl;

	this->socketBusy = false;

}

bool ReadSocket::isConnected(){

	return this->socketBusy;

	if(this->socketBusy==false)
		return false;

	this->write("isloggedin");

	char * temp = this->read();

	if(strcmp(temp,"yes")==0)
	{
		this->socketBusy = true;
		return true;
	}

	return false;
}


