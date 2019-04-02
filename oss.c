#include <stdio.h>
#include <stdlib.h>


#define CLOCK_INC 1000 //sample number, this is a constent. Experiment with it

struct PCB {
	//char data[100][80];
	int dummy;
};


int main(int argc, char *argv[]) {
	printf("Welcome to project 4\n");
	
	
	//we need a clock
	int clockSeconds = 0;
	int clockNano = 0;
	int clockInc = CLOCK_INC; 
	
	
	//we need our process control table
	int maxKidsAtATime = 3; //will be set by the "S" command line argument
	
	
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum blocks
	
	PCT[0].dummy = 4;
	printf("we stored %d in the dummy field\n", PCT[0].dummy);
	
	printf("End of program\n");
	return 0;
}