/*********************   CLASS DECLARATION    *********************/

class commandHandler {

	private:

		slaveHandler * sH;

	public:
		commandHandler(slaveHandler * sH); //constructor
		~commandHandler(void); //destructor

		void handleCommands(void);

};


/*********************   FUNCTIONS    *********************/


/** Constructor */
commandHandler::commandHandler(slaveHandler * sH) {

	this->sH = sH;

}

/** Destructor */
commandHandler::~commandHandler(void) {

}

void commandHandler::handleCommands(){

	std::string readT;

	char * read;

	bool running = true;

	while(running){


		//Process commands

		getline(std::cin, readT);

		read = (char*)readT.c_str();

		if(strcmp(read,"shutdown") == 0)
		{

			std::cout << "Shutdown command triggered, closing slaves..." << std::endl;

			sH->shutDownAgents();

			running = false;

		}
		else if(strcmp(read,"toggletable") == 0)
		{

			sH->toggleTable();

		}



	}

}
