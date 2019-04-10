#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include "messageQueue.h"
#include "queue.h"

#define CLOCK_INC 1000000000 //sample number, this is a constent. Experiment with it
#define MAX_TIME_BETWEEN_NEW_PROCESSES_NS 999999999
#define MAX_TIME_BETWEEN_NEW_PROCESSES_SECS 1 //we will create a new user process at a random interval between 0 and these values

#define TIMESLICE0 10
#define TIMESLICE1 2*TIMESLICE0
#define TIMESLICE2 2*TIMESLICE1
#define TIMESLICE3 2*TIMESLICE2
	

struct PCB {
	int myPID; //your local simulated pid
	int timeCreatedSecs;
	int timeCreatedNano;
	int totalCpuTimeUsed;
	int totalTimeInSystem;
	int timeUsedLastBurst;
	int processPriority; //0-3, start with only highest of 0, however
	int unblockSecs; //not sure what this default to, so be careful!!!
	int unblockNano;
};

struct blockedProcess {
	int myPID;
	int unblockSecs; //the seconds value that you can be unblocked
	int unblockNano; //the nanoseconds value that you can be unblocked
};

//generates a random number between 1 and 3
int randomNum(int max) {
	int num = (rand() % (max + 1));
	return num;
}

//we need a clock. These are set as globals since that allows us to easily implement an incrementClock function.
int clockSeconds = 0;
int clockNano = 0;
int clockInc = CLOCK_INC; 
void incrementClock(int inc) {
	clockNano += inc;
	if (clockNano >= 1000000000) { //increment the next unit
		clockSeconds += 1;
		clockNano -= 1000000000;
	}
	//printf("The current time is %d:%d\n", clockSeconds, clockNano);
}

//checks our boolArray for an open slot to save the process. Returns -1 if none exist
int checkForOpenSlot(bool boolArray[], int maxKidsAtATime) {
	int j;
	for (j = 0; j < maxKidsAtATime; j++) {
		if (boolArray[j] == false) {
			//printf("We have an open slot in %d\n", j);
			return j;
		} else {
			//printf("Slot %d is already full\n", j);
		}
	}
	return -1;
}

int blockedListOpenSlot(int blockedList[], int maxKidsAtATime) {
	int j;
	for (j = 0; j < maxKidsAtATime; j++) {
		if (blockedList[j] == 0) {
			return j;
		} else {
		}
	}
	return -1;
}

//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
	
	kill(-1*getpid(), SIGKILL);
}

int main(int argc, char *argv[]) {
	printf("Welcome to project 4\n");
	srand(time(0)); //placed here so we can generate random numbers later on

	//this section of code allows us to print the program name in error messages
	char programName[100];
	strcpy(programName, argv[0]);
	if (programName[0] == '.' && programName[1] == '/') {
		memmove(programName, programName + 2, strlen(programName));
	}
	
	//we need our process control table
	int maxKidsAtATime = 18; //will be set by the "S" command line argument. Default should probably be 18, but for us we are using 1 for early testing...
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum blocks
	int i;
	for (i = 0; i < maxKidsAtATime; i++) { //default all values to 0 to start
		PCT[i].myPID = 0; 
		PCT[i].timeCreatedSecs = 0;
		PCT[i].timeCreatedNano = 0;
		PCT[i].totalCpuTimeUsed = 0;
		PCT[i].totalTimeInSystem = 0;
		PCT[i].timeUsedLastBurst = 0;
		PCT[i].processPriority = 0;
		PCT[i].unblockSecs = 0; 
		PCT[i].unblockNano = 0;	
	}
	
	//our "bit vector" (or boolean array here) that will tell us which PRB are free
	bool boolArray[maxKidsAtATime]; //by default, these are all set to false. This has been tested and verified
	
	for (i = 0; i < maxKidsAtATime; i++) {
		boolArray[i] = false; //just set them all to false - quicker then checking
	}
	
	//set up our process queue
	struct Queue* queue0 = createQueue(maxKidsAtATime); //note: queue is the name of the queue. It is of type Queue, qhich is defined in queue.h. Pass in the max size
	struct Queue* queue1 = createQueue(maxKidsAtATime);
	struct Queue* queue2 = createQueue(maxKidsAtATime);
	struct Queue* queue3 = createQueue(maxKidsAtATime);
	
	//set up our blocked queue
	int blockedList[maxKidsAtATime]; 
	for (i = 0; i < sizeof(blockedList); i++) {
		blockedList[i] = 0; //set all values to 0 for starters
	}

	//set up our messageQueue
	key_t key; 
    int msgid; 
  
    key = 1094;
  
    // msgget creates a message queue and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT);  //create the message queue
	if (msgid < 0) {
		errorMessage(programName, "Error using msgget for message queue ");
	}
	//we are now ready to send messages whenever we desire
	
	//sample of the OSS loop
	int startSeconds, startNano, stopSeconds, stopNano, durationSeconds, durationNano;
	bool prepNewChild = false;
	
	int processesLaunched = 0;
	int processesRunning = 0;
	int done = 0;
	/*int stall = 0;
	int PL = 0;*/
	
	while (!done) {
		int temp = waitpid(-1, NULL, WNOHANG); //required to properly end processes and avoid a fork bomb
		if (temp < 0 && processesRunning > 0) {
			errorMessage(programName, "Unexpected result from terminating process ");
		}
		
		//first we check if we should prep a new child, and if so we do
		//printf("Is there a child preped? %d\n", prepNewChild);
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
			//we will try to launch our next child at stopSeconds:stopNano
			//printf("We have set a time to launch a new child\n");
		} 
		else if ((stopSeconds < clockSeconds) || ((stopSeconds == clockSeconds) && (stopNano < clockNano))) {
			if (processesLaunched < 100) { //for now, we have a placeholder of 10 processes in total per run of the code. THIS NEEDS TO CHANGE FOR FINAL SUBMISSION!!
				//time to launch the process
				
				//find out which PCT slot is free
				int openSlot = checkForOpenSlot(boolArray, maxKidsAtATime);
				if (openSlot != -1) { //a return value of -1 means all slots are currently filled, and per instructions we are to ignore this process


					//then set all values to that PCB - this still needs to be done
					//also actually launch the child
					pid_t pid;
					pid = fork();
					
					if (pid == 0) { //child
						execl ("user", "user", NULL);
						errorMessage(programName, "execl function failed ");
					}
					else if (pid > 0) { //parent
						boolArray[openSlot] = true; //first claim that spot
						//printf("boolArray[%d] was just set to true\n", openSlot);
						processesLaunched++;
						processesRunning++;
						//printf("Increased the number of processes running...\n");
						prepNewChild = false;
						
						//let's populate the control block with our data
						PCT[openSlot].myPID = pid;
						//note you may not need the next two lines... just experimenting
						PCT[openSlot].timeCreatedSecs = clockSeconds;
						PCT[openSlot].timeCreatedNano = clockNano;
						PCT[openSlot].totalCpuTimeUsed = 0;
						PCT[openSlot].totalTimeInSystem = 0;
						PCT[openSlot].timeUsedLastBurst = 0;
						if (randomNum(5) == 1) { //let's randomly determine if this child should be high priority or not
							printf("OSS : Generating process with PID %d and putting it in slot %d of PCT and queue #0 at time %d:%d\n", pid, openSlot, clockSeconds, clockNano);
							PCT[openSlot].processPriority = 0;
							enqueue(queue0, pid);
						} else {
							printf("OSS : Generating process with PID %d and putting it in slot %d of PCT and queue #1 at time %d:%d\n", pid, openSlot, clockSeconds, clockNano);
							PCT[openSlot].processPriority = 1;
							enqueue(queue1, pid);
						}
						continue;
					}
					else {
						errorMessage(programName, "Unexpected return value from fork ");
						kill(-1*getpid(), SIGKILL); //kills process and all children
					}
				}
			}
		}
		incrementClock(clockInc); //increment clock
		//check to see if we should start a process
		int whichQueueInUse;
		if (!isEmpty(queue0)) {
			whichQueueInUse = 0;
		} else if (!isEmpty(queue1)) {
			whichQueueInUse = 1;
		} else if (!isEmpty(queue2)) {
			whichQueueInUse = 2;
		} else if (!isEmpty(queue3)) {
			whichQueueInUse = 3;
		} else {
		    whichQueueInUse = -1;
		}
		
		if (whichQueueInUse != -1) {
			int nextPID;
			switch(whichQueueInUse) {
				case 0 :
					nextPID = dequeue(queue0);
					printf("OSS : Dispatching process with PID %d from queue #0 at time %d:%d\n", nextPID, clockSeconds, clockNano);
					break;
				case 1 :
					nextPID = dequeue(queue1);
					printf("OSS : Dispatching process with PID %d from queue #1 at time %d:%d\n", nextPID, clockSeconds, clockNano);
					break;
				case 2 :
					nextPID = dequeue(queue2);
					printf("OSS : Dispatching process with PID %d from queue #2 at time %d:%d\n", nextPID, clockSeconds, clockNano);
					break;
				case 3 :
					nextPID = dequeue(queue3);
					printf("OSS : Dispatching process with PID %d from queue #3 at time %d:%d\n", nextPID, clockSeconds, clockNano);
			}
				
			message.mesg_type = nextPID; //send a message to this specific child, and no other
			strncpy(message.mesg_text, "go", 100); //right now we're just sending "go" later on we'll have to be more specific
			message.return_address = getpid(); //tell them who sent it
				
			int send = msgsnd(msgid, &message, sizeof(message), 0);
			if (send == -1) {
				errorMessage(programName, "Error sending message via msgsnd command ");
			}
			//now we wait to receive a response
			int receive;
			int returnValue;
			receive = msgrcv(msgid, &message, sizeof(message), getpid(), 0); //wait for message back. OSS does nothing until then
			if (receive < 0) {
				errorMessage(programName, "Error receiving message via msgrcv command ");
			} else {
				returnValue = message.mesg_value; //we got a value back, and oss is in control again
			}
			
			if (returnValue > 0) { //we are done
				//deallocate resources and continue
				
				int j;
				for (j = 0; j < sizeof(PCT); j++) {
					if (PCT[j].myPID == nextPID) {
						boolArray[j] = false;
						PCT[j].myPID = 0; //remove PID value from this slot
						//remove everything else from this block of the table
						break;
					}
				}
				processesRunning--;
				//increment clock by correct amount of time
				double percentUsed = 0.1 * returnValue; //this should give us the percentage of time used
				int timeUsed; //therefore, we increment our clock by this value
				switch(whichQueueInUse) {
				case 0 :
					timeUsed = percentUsed * TIMESLICE0;
					break;
				case 1 :
					timeUsed = percentUsed * TIMESLICE1;
					break;
				case 2 :
					timeUsed = percentUsed * TIMESLICE2;
					break;
				case 3 :
					timeUsed = percentUsed * TIMESLICE3;
				}
				incrementClock(timeUsed);
				printf("OSS : Process with PID %d has finished at time %d:%d\n", nextPID, clockSeconds, clockNano);
			} else if (returnValue < 0) { //we are not done. We were blocked and that needs to be accounted for here
				//here is where we move it to the blocked list
				int open = blockedListOpenSlot(blockedList, maxKidsAtATime);
				if (open < 0) {
					perror("Error. Cannot block process. Too many processes exist within the system. System failure.\n");
					errorMessage(programName, "Error. Cannot block process. Too many processes exist within the system "); //should never get this, but just in case
				}
				blockedList[open] = nextPID; //save the pid in this location
									
				//now we need to know how long it should be inside the blocked list.
				//for the sake of simplicity, we'll use the same time data for when to start a new process.
				int blockSeconds = randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_SECS);
				int blockNano = randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_NS);
				//process will be blocked for blockSeconds:blockNano
				int blockStartSeconds = clockSeconds;
				int blockStartNano = clockNano;
				int blockStopSeconds = blockStartSeconds + blockSeconds;
				int blockStopNano = blockStartNano + blockNano;
				if (blockStopNano >= 1000000000) {
					blockStopSeconds += 1;
					blockStopNano -= 1000000000;
				}
				//process will be unblocked at blockStopSeconds:blockStopNano

				int j;
				for (j = 0; j < sizeof(PCT); j++) {
					if (PCT[j].myPID == nextPID) { //we find the PCB and save our stop time values there
						PCT[j].unblockSecs = blockStopSeconds;
						PCT[j].unblockNano = blockStopNano;
						//now we know when our process should be unblocked, and we have saved the values to the process control block
						break;
					}
				}
				
				//here is where we increment clock by the correct amount
				double percentUsed = -0.1 * returnValue; //this should give us the percentage of time used
				int timeUsed; //therefore, we increment our clock by this value
				switch(whichQueueInUse) {
				case 0 :
					timeUsed = percentUsed * TIMESLICE0;
					break;
				case 1 :
					timeUsed = percentUsed * TIMESLICE1;
					break;
				case 2 :
					timeUsed = percentUsed * TIMESLICE2;
					break;
				case 3 :
					timeUsed = percentUsed * TIMESLICE3;
				}
				incrementClock(timeUsed);
				printf("OSS : Process with PID %d was blocked and is being placed in the blocked array at time %d:%d\n", nextPID, clockSeconds, clockNano);				
			} else if (returnValue == 0) { //we are not done, but we used 100% of our time. Therefore, we were not blocked.
				//move it to the back of the queue
				
				switch(whichQueueInUse) {
				case 0 : //case 0 stays at highest priority
					enqueue(queue0, nextPID);
					incrementClock(TIMESLICE0);
					printf("OSS : Putting process with PID %d into queue #0\n", nextPID);
					break;
				case 1 : //all others take a step down
					enqueue(queue2, nextPID);
					incrementClock(TIMESLICE1);
					printf("OSS : Putting process with PID %d into queue #1\n", nextPID);
					break;
				case 2 :
					enqueue(queue3, nextPID);
					incrementClock(TIMESLICE2); //note that the queue and timeslice numbers do not match. This is not a mistakef
					printf("OSS : Putting process with PID %d into queue #2\n", nextPID);
					break;
				case 3 :
					enqueue(queue3, nextPID);
					incrementClock(TIMESLICE3);
					printf("OSS : Putting process with PID %d into queue #3\n", nextPID);
				}
			}
		}
		
		//now we check if there are any blocked processes we need to start up
		int k;
		for (k = 0; k < maxKidsAtATime; k++) {
			if (blockedList[k] > 0) {
				//this is a blocked process. Check if it should be unblocked or not
				int blockedPID = blockedList[k]; //store our blockedPID here
				int l;
				for (l = 0; l < maxKidsAtATime; l++) {
					if (PCT[l].myPID == blockedPID) { //if this is the blocked process, is it time to unblock it?
						if ((PCT[l].unblockSecs < clockSeconds) || ((PCT[l].unblockSecs == clockSeconds) && (PCT[l].unblockNano < clockNano))) {
							//if the time has passed, then we can unblock our process.
							if (PCT[l].processPriority == 0) {
								//this process is of the highest priority
								printf("OSS : Unblocking process with PID %d and placing it in queue #0 at time %d:%d\n", blockedPID, clockSeconds, clockNano);
								enqueue(queue0, blockedPID);
							} else {
								//this process is of a lower priority
								printf("OSS : Unblocking process with PID %d and placing it in queue #1 at time %d:%d\n", blockedPID, clockSeconds, clockNano);
								enqueue(queue1, blockedPID);
							}
							//deallocate all your blocked structure parts
							PCT[l].unblockSecs = 0;
							PCT[l].unblockNano = 0;
							blockedList[k] = 0;
						}
					}
				}
			}
		}

		//just for testing
		/*printf("SR\t%d:%d\t", clockSeconds, clockNano);
		int duh;
		for (duh = 0; duh < maxKidsAtATime; duh++) {
			printf("(%d, %d", duh, PCT[duh].myPID);
			if (PCT[duh].unblockSecs > 0) {
				printf(" BLOCKED) ");
			} else {
				printf(") ");
			}
		}
		printf("\n");
		
		//this is for teting as well
		printf("SR BA: ");
		int ba;
		for (ba = 0; ba < maxKidsAtATime; ba++) {
			printf(" %d ", boolArray[ba]);
		}
		printf("\n");*/
		
		printf("PL = %d\n", processesLaunched);
		
		
		//now we check to see if we are done
		if (processesLaunched > 99 && processesRunning == 0) { //terminates when we've launched and completed 10 processes.
			done = 1;
			printf("We are done with our loop\n");
		}
		
	}
	printf("We leave at %d:%d\n", clockSeconds, clockNano);
	
	//close message queue
	msgctl(msgid, IPC_RMID, NULL); 
	printf("That concludes this portion of the in-development program\n");
	printf("End of program\n");
	return 0;
}