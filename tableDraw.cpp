#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

using namespace std;

/*********************   CLASS DECLARATION    *********************/

class tableDraw {

	private:
		int columns;
		int lines;
		char * title;

		std::string ** matrix;
		int * matrixCellSize;

		int pos;

		bool titleIsSet;

	public:
		tableDraw(int columns, int lines); //constructor
		~tableDraw(void); //destructor

		void draw();
		void setTitle(char * title);
		void set(int line, int colunmn, char * value);

};


/*********************   FUNCTIONS    *********************/


/** Constructor */
tableDraw::tableDraw(int lines, int columns) {

	this->titleIsSet = false;

	this->columns = columns;
	this->lines = lines;

	this->matrix = new std::string*[this->lines];

	for( int i=0; i < this->lines; ++i )
	   this->matrix[i] = new std::string[this->columns];

	this->matrixCellSize = new int [this->columns];
	for( int i=0; i < this->columns; ++i )
		this->matrixCellSize[i]=0; 

}

/** Destructor */
tableDraw::~tableDraw(void) {

}

void tableDraw::setTitle(char * title){

	this->title = title;
	this->titleIsSet = true;

}

void tableDraw::set(int line, int column, char * value){
    
	int cellSize = strlen(value)+5;

	if(cellSize>this->matrixCellSize[column-1])
		this->matrixCellSize[column-1] = cellSize;

	this->matrix[line-1][column-1] = std::string(value);

}

void tableDraw::draw(){

	//Title

	if(this->titleIsSet)
		cout << this->title << endl;

	for(int i = 0; i< this->lines; i++)
	{

		//Header

		cout << setfill('-') << setw(1);

		for(int i2 =0; i2 < this->columns; i2++)
		{

			cout << "+" << setw(this->matrixCellSize[i2]) << "-" << setw(1);

		}

		cout << "+" << endl;

		for(int i2 =0; i2 < this->columns; i2++)
		{
			cout << setfill(' ') << setw(1) << "|" << setw(this->matrixCellSize[i2]) << left << this->matrix[i][i2];
		}
		
		cout << setw(1) << "|" << endl;

		//Footer

		if(i==this->lines-1)
		{

			cout << setfill('-') << setw(1);

			for(int i2 =0; i2 < this->columns; i2++)
			{

				cout << "+" << setw(this->matrixCellSize[i2]) << "-" << setw(1);

			}

			cout << "+" << endl;


		}


	}
}
