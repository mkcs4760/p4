#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define CLOCK_INC 1000 //sample number, this is a constent. Experiment with it
#define MAX_TIME_BETWEEN_NEW_PROCESSES_NS 999999999
#define MAX_TIME_BETWEEN_NEW_PROCESSES_SECS 1 //we will create a new user process at a random interval between 0 and these values


struct PCB {
	//char data[100][80];
	int dummy;
	int myPID; //your local simulated pid
	int totalCpuTimeUsed;
	int totalTimeInSystem;
	int timeUsedLastBurst;
	int processPriority; //0-3, start with only highest of 0, however
};

//generates a random number between 1 and 3
int randomNum(int max) {
	int num = (rand() % (max + 1));
	return num;
}


int main(int argc, char *argv[]) {
	printf("Welcome to project 4\n");
	srand(time(0)); //placed here so we can generate random numbers later on
	
	
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
	bool boolArray[maxKidsAtATime]; //by default, these are all set to false. This has been tested and verified
	int i;
	/*for (i = 0; i < sizeof(boolArray); i++) {
		printf("Is this PCB being used? %d \n", boolArray[i]);
	}*/
	
	//sample of the OSS loop
	for (i = 0; i < 10; i++) {
		printf("The current time is %d:%d\n", clockSeconds, clockNano);
		printf("Random wait time of the day is %d:%d\n", randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_SECS), randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_NS));
		//increment clock
		clockNano += clockInc;
		if (clockNano >= 1000000000) { //increment the next unit
			clockSeconds += 1;
			clockNano -= 1000000000;
		}
		//check to see if we should start a process
	}
	printf("We leave at %d:%d\n", clockSeconds, clockNano);
	
	
	printf("That concludes this portion of the in-development program");
	printf("To avoid warnings, let's use values: %d %d %d %d %d \n", PCT[0].dummy, boolArray[0], clockSeconds, clockNano, clockInc);
	printf("End of program\n");
	return 0;
}