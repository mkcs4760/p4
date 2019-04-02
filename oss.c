#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define CLOCK_INC 1000 //sample number, this is a constent. Experiment with it

struct PCB {
	//char data[100][80];
	int dummy;
	int myPID; //your local simulated pid
	int totalCpuTimeUsed;
	int totalTimeInSystem;
	int timeUsedLastBurst;
	int processPriority; //0-3, start with only highest of 0, however
};


int main(int argc, char *argv[]) {
	printf("Welcome to project 4\n");
	
	
	//we need a clock
	int clockSeconds = 0;
	int clockNano = 0;
	int clockInc = CLOCK_INC; 
	
	
	//we need our process control table
	int maxKidsAtATime = 18; //will be set by the "S" command line argument
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum blocks
	
	PCT[0].dummy = 4;
	//printf("we stored %d in the dummy field\n", PCT[0].dummy);
	
	//our "bit vector" (or boolean array here) that will tell us which PRB are free
	bool boolArray[maxKidsAtATime];
	int i;
	/*for (i = 0; i < sizeof(boolArray); i++) {
		boolArray[i] = false;
	}*/
	for (i = 0; i < sizeof(boolArray); i++) {
		printf("Is this PCB being used? %d \n", boolArray[i]);
	}
	
	printf("To avoid warnings, let's use values: %d %d %d %d \n", PCT[0].dummy, clockSeconds, clockNano, clockInc);
	printf("End of program\n");
	return 0;
}