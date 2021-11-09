#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <iomanip>
#include <thread>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <time.h>
#include "console-color.h"
#include "tableDraw.cpp"
#include "mysqlHandler.cpp"
#include "socket.cpp"
#include "slaveHandler.cpp"
#include "commandHandler.cpp"
 
using std::cout; using std::endl;
 
int main(int argc, char *argv[])
{
	
	//Welcome message :)

	std::cout << std::endl << "MD5 Brute Force - Master \n --------------------------------------------------------- \n Authors: Joao Vieira [info@joao-vieira.pt] & Rui Gamboias [rui.ismat@gmail.com] \n Description: - This engine uses brute force techniques to determine MD5 hash into strings.\nThis project was developed under the subject of 'Distributed Computation' of the ISMAT University @ Portugal.\n Year : ISMAT @ 2016\n --------------------------------------------------------- \n" << std::endl;

	//Slave Handler

	slaveHandler sH;

	sH.init();

	std::thread threadSHandler(&slaveHandler::handleSlaves, &sH);

	//Command Handler

	commandHandler cH(&sH);
	
	std::thread threadCHandler(&commandHandler::handleCommands, &cH);

	//Make main wait for shutdown

	threadSHandler.join();
	threadCHandler.join();

	return 0;

}
