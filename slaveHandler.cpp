#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <sstream>
#include <utility>

struct Slave {

	int curProcess;
	std::string ip;
	std::thread * slaveThread;
	ReadSocket * socket;

	std::string ipersec;
	std::string curPer;
	std::string timeleft;
	std::string status;

	bool match;
};


/*********************   CLASS DECLARATION    *********************/

class slaveHandler {

	private:

		mysqlHandler * dbHandler;

		char * bindIp;

		bool isPortOpen(char * hostname);

		std::vector<Slave*> slaves;

		bool shutdown;
		bool showTable;
		bool matchIsFound;

		bool useDB;
		bool clearSlaveDB;

		std::string slavePassword;
		std::string slaveUsername;

		std::vector<std::string> explode(std::string const & s, char delim);

		std::string hashToSearch;

		void handleMonitorThread(void);
		void matchFound(Slave * matchSlave);
	

	public:
		slaveHandler(void); //constructor
		~slaveHandler(void); //destructor

		void handleSlaves(void);
		void handleSlaveThread(Slave * slave);
		void setNewSlaveSequence(Slave * slave);
		void findSlaves(void);
		void shutDownAgents(void);
		void init(void);
		void toggleTable(void);

};


/*********************   FUNCTIONS    *********************/


/** Constructor */
slaveHandler::slaveHandler(void) {

	this->dbHandler = new mysqlHandler();
	this->shutdown = false;
	this->useDB = true;
	this->matchIsFound = false;
	this->clearSlaveDB = false;
	this->showTable = false;
}

/** Destructor */
slaveHandler::~slaveHandler(void) {

}

void slaveHandler::handleSlaves(){

	//Start slave threads

	std::cout << "Starting " <<  this->slaves.size() << " slave threads..." << std::endl;

	for (const auto& slave : this->slaves)
		slave->slaveThread = new std::thread(&slaveHandler::handleSlaveThread, this, slave);

	//Start table monitoring...

	std::thread monitorThread = std::thread(&slaveHandler::handleMonitorThread, this);

	//Join tasks

	for (const auto& slave : this->slaves)
		slave->slaveThread->join();

	monitorThread.join();

	std::cout << "All slaves disconected successfuly, master is shutting down..." << std::endl;
}

void slaveHandler::handleMonitorThread(void){


	auto t1 = std::chrono::high_resolution_clock::now();

	tableDraw * table1 = new tableDraw( this->slaves.size()+1,6);

	//Set title

	table1->setTitle((char *)"Peer Status");

	//Set headers name

	table1->set(1,1,(char *)"Host");
	table1->set(1,2,(char *)"I. p/sec");
	table1->set(1,3,(char *)"Seq.");
	table1->set(1,4,(char *)"Completed (%)");
	table1->set(1,5,(char *)"T.Left(min)");
	table1->set(1,6,(char *)"Status");


	while(this->shutdown==false)
	{

		auto t2 = std::chrono::high_resolution_clock::now();

		if(std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count()>=5000)
		{

			int i=2;

			for (const auto& slave : this->slaves)
			{

				table1->set(i,1,(char*)slave->ip.c_str());
				table1->set(i,2,(char*)slave->ipersec.c_str());
				table1->set(i,3,(char*)std::to_string(slave->curProcess).c_str());
				table1->set(i,4,(char*)slave->curPer.c_str());
				table1->set(i,5,(char*)slave->timeleft.c_str());
				table1->set(i,6,(char*)slave->status.c_str());

				i++;

			}

			if(this->showTable==true)
			{

				//Clear screen

				cout << string( 200, '\n' );

				table1->draw();

			}

			//Reset clock

			t1 = std::chrono::high_resolution_clock::now();

		}


	}


}

void slaveHandler::handleSlaveThread(Slave * slave){

	//Init Socket

	slave->socket = new ReadSocket();

	auto t1 = std::chrono::high_resolution_clock::now();
	auto t2 = std::chrono::high_resolution_clock::now();

	while(this->shutdown==false)
	{

		//Execute every 5 seconds...

		t2 = std::chrono::high_resolution_clock::now();

		if(std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count()>=5000)
		{

			//Check if is connected

			if(slave->socket->isConnected() == false )
			{

				//Connect to slave...

				slave->socket->connect((char *)slave->ip.c_str());
				slave->socket->read();

				//Authenticating...

				slave->socket->read();
				slave->socket->write("admin");
				slave->socket->write("password");

				if(strcmp(slave->socket->read(),"Authentication successfull!")!=0)
				{

					slave->socket->close();
					continue;

				}
							
/*

				if(slave->socket->isConnected() == false )
				{
					slave->socket->close();
					continue;
				}*/

				//Go raw

				slave->socket->write("goraw");
				slave->socket->read();

				//Truncate slave

				if(this->clearSlaveDB)
				{

					slave->socket->write("truncate");
					slave->socket->read();
					slave->socket->write("y");
					slave->socket->read();

				}

				//Use db

				if(!this->useDB)
				{

					slave->socket->write("usesql");
					slave->socket->read();
					slave->socket->write("n");
					slave->socket->read();

				}

				//Reset Slave

				slave->socket->write("reset");
				slave->socket->read();

				//Set Md5 hash

				slave->socket->write("setmd5");
				slave->socket->read();
				slave->socket->write(this->hashToSearch.c_str());

				//Reset clock
				
				t1 = std::chrono::high_resolution_clock::now();

				continue;

				
			}

			slave->socket->write("status");

			char * temp = slave->socket->read();

			//std::cout << "Readed[" << temp << "]" <<std::endl;

			if(strlen(temp)>0)
			{
				//If we have a string with something...

				auto v = explode(temp, '|');

				slave->status = v[0];

				//Process commands

				if(strcmp(v[0].c_str(),"wait4sec") == 0 && this->matchIsFound==false)
				{
			
					std::cout << "(" << slave->ip << ") - Is waitting for sequence..." << std::endl;

					this->setNewSlaveSequence(slave);


				}

				if(strcmp(v[0].c_str(),"running") == 0)
				{
			
					slave->ipersec = v[1];
					slave->curPer = v[2];
					slave->timeleft = v[3]; 


				}

				if(strcmp(v[0].c_str(),"matchfound") == 0 && slave->match==false)
				{

					slave->match = true;

					//Handle results

					this->matchFound(slave);			

				}

			}

			//Reset clock

			t1 = std::chrono::high_resolution_clock::now();

		}

	
	}


	//Shutdown slave...

	if(this->shutdown)
	{

		slave->socket->write("shutdown");

		std::cout << "(" << slave->ip << ") - Shutdown command sent, closing connection..." << std::endl;

		slave->socket->close();

	}

	

}

void slaveHandler::matchFound(Slave * matchSlave){

	//Set flag

	this->matchIsFound = true;

	//Get match

	matchSlave->socket->read();
	matchSlave->socket->write("getmatch");
	std::string match = matchSlave->socket->read();


	//Stop all slaves from processing...

	for (const auto& slave : this->slaves)
	{

		slave->socket->write("reset");
		slave->socket->read();

	}

	//Print results to console

	this->showTable = false;

	std::cout << string( 200, '\n' ) << termcolor::green << " ---------------------- !!! SUCCESS !!! ---------------------- " << std::endl << termcolor::white <<" A match was found by a slave!" << std::endl;

	tableDraw * table1 = new tableDraw(2,3);

	//Set headers name

	table1->set(1,1,(char *)"Found by host");
	table1->set(1,2,(char *)"Hash");
	table1->set(1,3,(char *)"--- Match Found ---");

	table1->set(2,1,(char*)matchSlave->ip.c_str());
	table1->set(2,2,(char*)this->hashToSearch.c_str());
	table1->set(2,3,(char*)match.c_str());

	table1->draw();

}



std::vector<std::string> slaveHandler::explode(std::string const & s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(std::move(token));
    }

    return result;
}

void slaveHandler::init(){

	std::cout << "Initializing engine..." << std::endl;

	//Check which interface to bind...

	struct ifaddrs * ifAddrStruct=NULL;
	struct ifaddrs * ifa=NULL;

	void * tmpAddrPtr=NULL;

	bool allocated = false;

	char read = 's';

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
		    continue;
		}
		if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
			// is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

			if(strcmp(addressBuffer,"127.0.0.1") != 0)
			{

				//Already has address make the question

				read = 's';

				do
				{
					std::cout << "Would you like to bind to " << ifa->ifa_name << ":" << addressBuffer << " address? [y,n]" << std::endl;

    					std::cin>>read;

					if(read=='y')
					{
						this->bindIp = addressBuffer;
						allocated = true;
					}

				}while(read!='y' && read!='n');

				if(this->bindIp!=NULL)
					break;


			}

		}
	}

	//Check if binded is set, otherwise repeat...

	if(!allocated)
	{
		this->init();
		return;

	}

	//Prompt for MD5 string

	read = 's';

	do
	{
		std::cout << "Use MD5 test string '4444' - dbc4d84bfcfe2284ba11beffb853a8c4 ? [y,n]" << std::endl;

		std::cin>>read;

		if(read=='n')
		{
			std::cout << "Provide an md5 to search then:" << std::endl;

		    	std::cin>>this->hashToSearch; 

		}
		if(read=='y')
			this->hashToSearch = "dbc4d84bfcfe2284ba11beffb853a8c4";


	}while(read!='y' && read!='n');

	std::cout << "Setting hash" << this->hashToSearch << " as search md5..." << std::endl;
	
	//Prompt for authentication

	read = 's';

	do
	{
		std::cout << "Use standard username and password to login on slaves? [y,n]" << std::endl;

		std::cin>>read;

		if(read=='n')
		{
			std::cout << "Provide the username:" << std::endl;
			
			std::cin.ignore();

			std::getline(std::cin, this->slaveUsername);

			std::cout << "Provide the password:" << std::endl;
	
		    	std::getline(std::cin, this->slavePassword);

		}
		if(read=='y')
		{
			this->slaveUsername = (char*)"admin";
			this->slavePassword = (char*)"password";

		}


	}while(read!='y' && read!='n');

	std::cout << "Setting hash" << this->hashToSearch << " as search md5..." << std::endl;

	//Prompt to enable database

	read = 's';

	do
	{
		std::cout << "Enable database handling on the slaves? [y,n]" << std::endl;

		std::cin>>read;

		if(read=='n')
			this->useDB = false;


	}while(read!='y' && read!='n');

	//Prompt to erase all records from slaves

	read = 's';

	do
	{
		std::cout << "Clear slaves databases? [y,n]" << std::endl;

		std::cin>>read;

		if(read=='y')
		{
			//Set flag

			this->clearSlaveDB = true;

			//Truncate mysql records on master

			std::cout << "Clearing master sequence records" << std::endl;

			std::string queryStatement = "truncate sequences;";

			this->dbHandler->query((char*)queryStatement.c_str());

		}


	}while(read!='y' && read!='n');

	//Find slaves over the net

	this->findSlaves();

}

bool slaveHandler::isPortOpen(char * ip)
{
	
	struct sockaddr_in address; 
	short int sock = -1;
	int port = 3000;
	fd_set fdset;
	struct timeval tv;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
	address.sin_port = htons(port);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock, F_SETFL, O_NONBLOCK);

	connect(sock, (struct sockaddr *)&address, sizeof(address));

	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	tv.tv_sec = 1;
	tv.tv_usec = 1;

	if (select(sock + 1, NULL, &fdset, NULL, &tv) == 1)
	{
		int so_error;
		socklen_t len = sizeof so_error;

		getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);

		if (so_error == 0){
			close(sock);
			return true;

		}else{
			close(sock);
			return false;
		}
	}   
     
	return false;

}

void slaveHandler::findSlaves(){

	//Stage 1 - Look for peers on the subnet...

	std::cout << "Checking for slaves listening on port 3000 across the subnet..." << std::endl;

	std::string ip_prefix = std::string(this->bindIp);

	std::string token = ip_prefix.substr(0, ip_prefix.find_last_of("."));

	int lookUpTo = 0;
	
	std::cout << "How many slaves to look for? [0=unknown]" << std::endl;

	std::cin>>lookUpTo;

	for (int i=1; i<254; i++){ 
     
		std::ostringstream ip;
		ip << token << "." << i;

		std::string tmp = ip.str();

		std::cout << "Probbing IP [" << tmp << "] on port 3000..." << "\r";

		std::cout.flush();

		if(this->isPortOpen((char *)tmp.c_str()))
		{
			//Port is open

			std::cout << std::endl << "Port is open on " << tmp << ", adding slave." << std::endl;

			Slave * slave = new Slave;

			slave->ip = tmp;
			slave->curProcess = -1;
			slave->ipersec = "---";
			slave->curPer = "---";
			slave->timeleft = "---";
			slave->status = "---"; 
			slave->match = false; 

			this->slaves.push_back(slave);

		}
		else
		{
			std::cout << "Port is closed on " << tmp << "." << "\r";
			std::cout.flush();
		}

		if((signed int)this->slaves.size()>=lookUpTo && lookUpTo>=0)
			break;

	}
		

	if(this->slaves.size()<=0)
	{

		std::cout << "No slaves found in the subnet, engine cannot continue..." << std::endl;
		std::cout << "Shutting down..." << std::endl;
	
		exit(0);

	}

}

void slaveHandler::setNewSlaveSequence(Slave * slave){

	//Check for current processing...

	std::string queryStatement = "";

	if(slave->curProcess>=1)
	{

		//We assume that has been processed and set the correspondent row as completed...

		std::cout << "(" << slave->ip << ") - Sequence [" <<  slave->curProcess << "] is completed" << std::endl;

		queryStatement = "update sequences set completed=1 where ip=':ip' and seq=':seq'";

		this->dbHandler->setField(queryStatement, ":ip", slave->ip.c_str());
		this->dbHandler->setField(queryStatement, ":seq", slave->curProcess);

		this->dbHandler->query((char*)queryStatement.c_str());

	}

	sql::ResultSet * res;

	queryStatement = "select * from sequences where ip=':ip' and completed=0 order by seq desc limit 1";

	this->dbHandler->setField(queryStatement, ":ip", slave->ip.c_str());

	res = this->dbHandler->query((char*)queryStatement.c_str());

	if(res->rowsCount()>=1)
	{

		while (res->next()) {

			//Set sequence on slave...

			slave->socket->write("setsequence");
			slave->socket->read();
			slave->socket->write(res->getString("seq").c_str());
			slave->socket->read();

			slave->curProcess = atoi(res->getString("seq").c_str());

			std::cout << "(" << slave->ip << ") - Resuming sequence [" << res->getString("seq") << "] process on this slave" << std::endl;

			return;


		}
		

	}

	//Get new sequence and add it to db...

	queryStatement = "select * from sequences order by seq desc limit 1";

	res = this->dbHandler->query((char*)queryStatement.c_str());

	if(res->rowsCount()>=1)
	{


		while (res->next()) {

			//Star increment

			std::string newSeq = res->getString("seq");
			int nsq = atoi(newSeq.c_str());
			newSeq = std::to_string(nsq+1);

			//Insert into mysql

			queryStatement = "insert into sequences values(NULL,':ip',':seq',0);";

			this->dbHandler->setField(queryStatement, ":ip", slave->ip.c_str());
			this->dbHandler->setField(queryStatement, ":seq", newSeq.c_str());

			this->dbHandler->query((char*)queryStatement.c_str());

			//Set sequence on slave...

			slave->socket->write("setsequence");
			slave->socket->read();
			slave->socket->write(newSeq.c_str());
			slave->socket->read();

			slave->curProcess = atoi(newSeq.c_str());

			std::cout << "(" << slave->ip << ") - Setting new sequence [" << newSeq << "] process on this slave" << std::endl;


		}

	}
	else
	{

		//Start as one seq

		std::string newSeq = "1";

		//Insert into mysql

		queryStatement = "insert into sequences values(NULL,':ip',':seq',0);";

		this->dbHandler->setField(queryStatement, ":ip", slave->ip.c_str());
		this->dbHandler->setField(queryStatement, ":seq", newSeq.c_str());

		this->dbHandler->query((char*)queryStatement.c_str());

		//Set sequence on slave...

		slave->socket->write("setsequence");
		slave->socket->read();
		slave->socket->write(newSeq.c_str());
		slave->socket->read();

		slave->curProcess = atoi(newSeq.c_str());

		std::cout << "(" << slave->ip << ") - Setting new sequence [" << newSeq << "] process on this slave" << std::endl;

	}

	

}

void slaveHandler::shutDownAgents(void){

	if(this->shutdown==false)
		this->shutdown = true;

}

void slaveHandler::toggleTable(void){

	if(this->showTable==false)
		this->showTable = true;
	else
		this->showTable = false;

}



