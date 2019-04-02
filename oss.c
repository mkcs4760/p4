#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define CLOCK_INC 100000000 //sample number, this is a constent. Experiment with it
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

int checkForOpenSlot(bool boolArray[], int maxKidsAtATime) {
	int j;
	for (j = 0; j < maxKidsAtATime; j++) {
		if (boolArray[j] == false) {
			printf("We have an open slot in %d\n", j);
			return j;
		} else {
			printf("Slot %d is already full\n", j);
		}
	}
	return -1;
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
	for (i = 0; i < sizeof(boolArray); i++) {
		printf("Is PCB #%d being used? %d \n", i, boolArray[i]);
	}
	
	//sample of the OSS loop
	int startSeconds, startNano, stopSeconds, stopNano, durationSeconds, durationNano;
	bool prepNewChild = false;
	
	
	for (i = 0; i < 300; i++) {
		printf("The current time is %d:%d\n", clockSeconds, clockNano);
		//printf("Random wait time of the day is %d:%d\n", randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_SECS), randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_NS));
		//printf("is there a child prepped? %d\n", prepNewChild);
		if (prepNewChild == false) {
			prepNewChild = true;
			durationSeconds = randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_SECS);
			durationNano = randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_NS);
			startSeconds = clockSeconds;
			startNano = clockNano;
			
			stopSeconds = startSeconds + durationSeconds;
			stopNano = startNano + durationNano;
			if (stopNano >= 1000000000) {
				stopSeconds += 1;
				stopNano -= 1000000000;
			}
			printf("Let's start the next child at %d:%d\n", stopSeconds, stopNano);
		} else {
			if ((stopSeconds < clockSeconds) || ((stopSeconds == clockSeconds) && (stopNano < clockNano))) {
				//time to launch the process
				printf("Launch time!\n");
				prepNewChild = false;
				
				//find out which PCT slot is free
				int openSlot = checkForOpenSlot(boolArray, maxKidsAtATime);
				if (openSlot == -1) {
					printf("All slots are full. Ignore this process\n");
				} else {
					printf("We can store it in slot %d\n", openSlot);
					//now actually store it in said slot
					boolArray[openSlot] = true; //first claim that spot
					//then set all values to that PCB - this still needs to be done
				}
		
			}
		}
		//increment clock
		clockNano += clockInc;
		if (clockNano >= 1000000000) { //increment the next unit
			clockSeconds += 1;
			clockNano -= 1000000000;
		}
		//check to see if we should start a process
	}
	printf("We leave at %d:%d\n", clockSeconds, clockNano);
	
	
	printf("That concludes this portion of the in-development program\n");
	printf("To avoid warnings, let's use values: %d \n", PCT[0].dummy);
	printf("End of program\n");
	return 0;
}