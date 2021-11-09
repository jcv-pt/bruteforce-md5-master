#include <stdlib.h>
#include <iostream>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

/*********************   CLASS DECLARATION    *********************/

class mysqlHandler {

	private:

		char * database;
		char * username;
		char * password;

		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;
		sql::ResultSet *res;

		void raiseError(sql::SQLException &e, char * query);


	public:
		mysqlHandler(void); //constructor
		~mysqlHandler(void); //destructor

		sql::ResultSet * query(char * query);

		bool setField(std::string& str, const std::string& from, const std::string& to);
		bool setField(std::string& str, const std::string& from, const int to);

};


/*********************   FUNCTIONS    *********************/


/** Constructor */
mysqlHandler::mysqlHandler(void) {

	//Init vars

	this->database = (char*) "md5DB";
	this->username = (char*) "root";
	this->password = (char*) "root";

	//Connect to DB

	try {

	  /* Create a connection */

	  this->driver = get_driver_instance();

	  this->con = this->driver->connect("tcp://127.0.0.1:3306", this->username, this->password);

	  /* Connect to the MySQL database */

	  con->setSchema(this->database);


	} catch (sql::SQLException &e) {
		this->raiseError(e, NULL);
	}

}

/** Destructor */
mysqlHandler::~mysqlHandler(void) {

	this->con->close();

	delete this->res;
	delete this->stmt;
	delete this->con;

}

sql::ResultSet * mysqlHandler::query(char * query) {

	try {

		this->stmt = this->con->createStatement();
		this->res = this->stmt->executeQuery(query);

	} catch (sql::SQLException &e) {

		this->raiseError(e, query);
		
	}

	return this->res;

}

bool mysqlHandler::setField(std::string& str, const std::string& from, const std::string& to) {
    
	size_t start_pos = str.find(from);
	
	if(start_pos == std::string::npos)
		return false;

	str.replace(start_pos, from.length(), to);

	return true;
}

bool mysqlHandler::setField(std::string& str, const std::string& from, const int to) {
    

	std::string to_ = std::to_string(to);
	
	return this->setField(str, from ,to_);
}

void mysqlHandler::raiseError(sql::SQLException &e, char * query){

	if(e.getErrorCode() ==0)
		return;

	std::cout << "# ERR: SQLException" << std::endl << e.what()<< std::endl;
	std::cout << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl << "Query as follows:" << std::endl;
	std::cout << query << std::endl;
	exit(1);

}
