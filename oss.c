#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
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
	//char data[100][80];
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
	printf("The current time is %d:%d\n", clockSeconds, clockNano);
}

//checks our boolArray for an open slot to save the process. Returns -1 if none exist
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

int blockedQueueOpenSlot(int blockedQueue[], int maxKidsAtATime) {
	int j;
	for (j = 0; j < maxKidsAtATime; j++) {
		if (blockedQueue[j] == 0) {
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
	
	//set up our time values
	//int timeSlice0 = 10;
	//int timeSlice1 = 2 * timeSlice0;
	//int timeSlice2 = 2 * timeSlice1;
	//int timeSlice3 = 2 * timeSlice2;
	
	
	
	//we need our process control table
	int maxKidsAtATime = 2; //will be set by the "S" command line argument. Default should probably be 18, but for us we are using 1 for early testing...
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
	
	for (i = 0; i < sizeof(boolArray); i++) {
		//printf("Is PCB #%d being used? %d \n", i, boolArray[i]);
		if (boolArray[i] != 0) {
			boolArray[i] = false; //JUST CHANGE THIS TO SET THEM ALL TO FALSE - QUICKER THEN CHECKING
			//printf("Is PCB #%d being used? %d \n", i, boolArray[i]);
		}
	}
	
	//set up our process queue
	struct Queue* queue = createQueue(maxKidsAtATime); //note: queue is the name of the queue. It is of type Queue, qhich is defined in queue.h. Pass in the max size
	
	//set up our blocked queue
	//struct Queue* blockedQueue = createQueue(maxKidsAtATime); //blocked processes are still running, so to avoid forkbombs, we should not allow more then the max
	struct blockedProcess BP[maxKidsAtATime]; //this array contains the times that blocked processes can start up again
	//I DO NOT BELIEVE THE LINE ABOVE NOR THE STRUCTURE IT USES ARE NEEDED FOR THIS PROGRAM. TEST BEFORE DELETING
	int blockedQueue[maxKidsAtATime];
	for (i = 0; i < sizeof(blockedQueue); i++) {
		blockedQueue[i] = 0; //set all values to 0 for starters
	}

	//set up our messageQueue
	key_t key; 
    int msgid; 
  
    key = 1094;
  
    // msgget creates a message queue and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT);  //create the message queue
	if (msgid < 0) {
		printf("Error, msgid equals %d\n", msgid);
	}
	//we are now ready to send messages whenever we desire
	
	//sample of the OSS loop
	int startSeconds, startNano, stopSeconds, stopNano, durationSeconds, durationNano;
	bool prepNewChild = false;
	
	int processesLaunched = 0;
	int processesRunning = 0;
	int done = 0;
	
	while (!done) {
		//printf("The current time is %d:%d\n", clockSeconds, clockNano);
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
		} 
		else if ((stopSeconds < clockSeconds) || ((stopSeconds == clockSeconds) && (stopNano < clockNano))) {
			if (processesLaunched < 10) {
				//time to launch the process
				printf("It is time to try to launch a new process\n");
				prepNewChild = false;
				
				//find out which PCT slot is free
				int openSlot = checkForOpenSlot(boolArray, maxKidsAtATime);
				if (openSlot == -1) {
					printf("All slots are full. Ignore this launch attempt\n");
				} else {
					printf("We can store a new process in slot %d\n", openSlot);
					processesLaunched++;
					processesRunning++;
					//now actually store it in said slot
					boolArray[openSlot] = true; //first claim that spot
					//then set all values to that PCB - this still needs to be done
					//also actually launch the child
					pid_t pid;
					pid = fork();
					
					if (pid == 0) { //child
						execl ("user", "user", NULL);
						perror("execl failed. \n");
					}
					else if (pid > 0) { //parent
						printf("OSS: Generating process with PID %d and putting it in queue %d at time %d:%d\n", pid, 0, clockSeconds, clockNano);
						//the above is the official log statement to print to file. Note, "0" must be replaced with queue number, and "OSS" should come form a variable and not be hardcoded into program
						
						//let's populate the control block with our data
						PCT[openSlot].myPID = pid;
						PCT[openSlot].processPriority = 0; //for now, we'll just use one highest priority queue.....
						//note you may not need the next two lines... just experimenting
						PCT[openSlot].timeCreatedSecs = clockSeconds;
						PCT[openSlot].timeCreatedNano = clockNano;
						PCT[openSlot].totalCpuTimeUsed = 0;
						PCT[openSlot].totalTimeInSystem = 0;
						PCT[openSlot].timeUsedLastBurst = 0;
						enqueue(queue, pid); //add this process to the queue
						//this should be everything set up.
						
						//oringinally sent message here. Moved it down below
						continue;
					}
					else {
						//Error
					}
				}
			}
		}
		incrementClock(clockInc); //increment clock
		//check to see if we should start a process
		if (!isEmpty(queue)) {
			int whichQueueInUse = 0; //this value will vary 0-3 depending upon which queue we are launching a process from
			//queue is not empty. Thus we should run something
			int nextPID = dequeue(queue); //decide the next process to run. Here it's simple with only one queue. Later this will require more attention
			
			//let's send this process a message
			message.mesg_type = nextPID; //send a message to this specific child, and no other
			strncpy(message.mesg_text, "go", 100); //right now we're just sending "go" later on we'll have to be more specific
			message.return_address = getpid(); //tell them who sent it
			// msgsnd to send message 
			//printf("child will do nothing until we let it\n");
			
			printf("OSS : Dispatching process with PID %d from queue %d at time %d:%d\n", nextPID, 0, clockSeconds, clockNano);
			//the above is the official log statement to print to file. Note, "0" must be replaced with queue number, and "OSS" should come form a variable and not be hardcoded into program
			
			int send = msgsnd(msgid, &message, sizeof(message), 0);
			if (send == -1) {
				perror("Error on msgsnd\n");
			}
			//now we wait to receive a response
			int receive;
			int returnValue;
			receive = msgrcv(msgid, &message, sizeof(message), getpid(), 0); //wait for message back. OSS does nothing until then
			if (receive < 0) {
				perror("No message received\n");
			} else {
				printf("Parent received this message from %d : %s \n", nextPID, message.mesg_text); //later on we'll have to process the response
				returnValue = message.mesg_value;
				printf("Parent is now in control again\n");
			}
			//determine what to do with this pid. If it's done, do nothing. Else, send it back in.
			//int choice = randomNum(2); //note, the random stuff will have to happen in user, not here. But for testing today, let's do it here
			if (returnValue > 0) { //we are done
				//deallocate resources and continue
				printf("Process %d was finished\n", nextPID);
				//find out which boolArray position needs to be free
				int j;
				for (j = 0; j < sizeof(PCT); j++) {
					if (PCT[j].myPID == nextPID) {
						boolArray[j] = 0;
						PCT[j].myPID = 0; //remove PID value from this slot
						//remove everything else from this block of the table
						printf("We are deallocating the value at boolArray[%d]\n", j);
						break;
					}
				}
				processesRunning--;
				//increment clock by correct amount of time
				if (whichQueueInUse == 0) {
					double percentUsed = 0.1 * returnValue; //this should give us the percentage
					printf("This process has used %f of it's time slice.\n", percentUsed);
					int timeUsed = percentUsed * TIMESLICE0;
					printf("Therefore, we will increment our clock by %d\n", timeUsed);
					incrementClock(timeUsed);
					
				}
				//include values for the other queues
				//printf("Return value was %d, meaning we need to increment our clock by %d0 percent of the value sent to the child\n", returnValue, returnValue);
				//NOTE: CAN'T REALLY DO THIS UNTIL THE PARENT ACTUALLY SENDS TIME VALUE TO CHILD - SO MAKE THAT THE NEXT STEP!!
				//THEN GO BACK AND INCREMENT THE CLOCK HERE!!
			} else if (returnValue < 0) { //we are not done. We were blocked and that needs to be accounted for here
				printf("Process %d was not finished. It was blocked\n", nextPID);
				//here is where we move it to the blocked queue. For now, we just add it back to the same queue
				int open = blockedQueueOpenSlot(blockedQueue, maxKidsAtATime);
				if (open < 0) {
					perror("Error. Cannot block process. Too many processes exist within the system. System failure.\n"); //terminate program here
				}
				printf("BLOCKING PROCESS %d\n", nextPID);
				blockedQueue[open] = nextPID; //save the pid in this location
									
				//enqueue(blockedQueue, nextPID); //here we add the process to the blocked queue
				//now we need to know how long it should be inside the blocked queue.
				//for the sake of simplicity, we'll use the same time data for when to start a new process.
				printf("Let's do some math. Current time is %d:%d\n", clockSeconds, clockNano);
				int blockSeconds = randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_SECS);
				int blockNano = randomNum(MAX_TIME_BETWEEN_NEW_PROCESSES_NS);
				printf("Process %d will be blocked for %d:%d\n", nextPID, blockSeconds, blockNano);
				int blockStartSeconds = clockSeconds;
				int blockStartNano = clockNano;
				printf("According to this, we say %d = %d + %d\n", blockStartSeconds + blockSeconds, blockStartSeconds, blockSeconds);
				int blockStopSeconds = blockStartSeconds + blockSeconds;
				int blockStopNano = blockStartNano + blockNano;
				if (blockStopNano >= 1000000000) {
					blockStopSeconds += 1;
					blockStopNano -= 1000000000;
				}
				printf("Process %d will be unblocked at %d:%d\n", nextPID, blockStopSeconds, blockStopNano);
				
				//find out which slot of the PCT we need to save our unblock time
				//}

				int j;
				for (j = 0; j < sizeof(PCT); j++) {
					if (PCT[j].myPID == nextPID) {
						PCT[j].unblockSecs = blockStopSeconds;
						PCT[j].unblockNano = blockStopNano;
						//now we know when our process should be unblocked, and we have saved the values to the process control block
						printf("We just saved the value %d:%d in PCT[%d] under PID %d\n", blockStopSeconds, blockStopNano, j, nextPID);
						printf("We just saved the value %d:%d in PCT[%d] under PID %d\n", PCT[j].unblockSecs, PCT[j].unblockNano, j, nextPID);
						break;
					}
				}
				
				
				//here is where we increment clock by the correct amount
				if (whichQueueInUse == 0) {
					double percentUsed = -0.1 * returnValue; //this should give us the positive percentage
					printf("This process has used %f of it's time slice.\n", percentUsed);
					int timeUsed = percentUsed * TIMESLICE0;
					printf("Therefore, we will increment our clock by %d\n", timeUsed);
					incrementClock(timeUsed);
				}
				//THE LINES ABOVE NEED TO BE IMPLEMENTED ONCE OTHER FEATURES ARE IMPLEMENTED	
			} else if (returnValue == 0) { //we are not done, but we used 100% of our time. Therefore, we were not blocked.
				printf("Process %d was not finished, but used all of it's name\n", nextPID);
				//move it to the back of the queue
				enqueue(queue, nextPID);//add again to the back of the queue
				printf("OSS : Putting process with PID %d into queue %d\n", nextPID, 0);
				//the above is the official log statement to print to file. Note, "0" must be replaced with queue number, and "OSS" should come form a variable and not be hardcoded into program
				//THIS IS WHERE WE INCREMENT THE CLOCK BY THE CORRECT AMOUNT OF TIME
				if (whichQueueInUse == 0) {
					incrementClock(TIMESLICE0); //this increments the clock
				}
			} else {
				perror("ERROR! Invalid return value!");
			}

		}
		
		//now we check if there are any blocked processes we need to start up
		printf("Let's check if there is anything to unblock!!\n");
		int k;
		for (k = 0; k < maxKidsAtATime; k++) {
			if (blockedQueue[k] > 0) {
				//this is a blocked process. Check if it should be unblocked or not
				int blockedPID = blockedQueue[k]; //store our blockedPID here
				int l;
				printf("we recall that process %d is blocked in slot %d of our blockedQueue\n", blockedPID, k);
				for (l = 0; l < sizeof(PCT); l++) {
					if (PCT[l].myPID == blockedPID) { //if this is the blocked process
						//we have the blocked process. Now we compare it's end time with the current time
						printf("We have found that this blocked process %d is stored in slot %d of the PCT\n", blockedPID, l);
						if ((PCT[l].unblockSecs < clockSeconds) || ((PCT[l].unblockSecs == clockSeconds) && (PCT[l].unblockNano < clockNano))) {
							//if the time has passed, then we can unblock our process.
							printf("We are in PCT[%d]\n", l);
							printf("Looks like process %d is ready to be unblocked, since it was free at %d:%d and it is now %d:%d\n", blockedPID, PCT[l].unblockSecs, PCT[l].unblockNano, clockSeconds, clockNano);
							printf("UNBLOCKING PROCESS %d\n", blockedPID);
							//deallocate all your blocked structure parts
							PCT[l].unblockSecs = 0;
							PCT[l].unblockNano = 0;
							blockedQueue[k] = 0;
							//add the process back to the running queue
							enqueue(queue, blockedPID);
							//I think that's about it
						}
						else {
							printf("We will not unblock process %d, since it will be free at %d:%d and it is now %d:%d\n", blockedPID, PCT[l].unblockSecs, PCT[l].unblockNano, clockSeconds, clockNano);
							
						}
					}
				}
			}
		}

		//just for testing
		printf("!!!Let's check all values within our PCT!\n");
		int duh;
		for (duh = 0; duh < maxKidsAtATime; duh++) {
			printf("(%d, %d", duh, PCT[duh].myPID);
			if (PCT[duh].unblockSecs > 0) {
				printf(" %d) ", PCT[duh].unblockSecs);
			} else {
				printf(") ");
			}
		}
		
		printf("\n");
		
		//process return value and continue
		//loopCount++;
		//now we check to see if we are done
		if (processesLaunched > 4 && processesRunning == 0) { //terminates when we've launched and completed 10 processes.
			done = 1;
			printf("We are done with our loop\n");
		}
	}
	printf("We leave at %d:%d\n", clockSeconds, clockNano);
	
	//close message queue
	msgctl(msgid, IPC_RMID, NULL); 
	printf("That concludes this portion of the in-development program\n");
	printf("To avoid warnings, let's use values: %d \n", PCT[0].myPID);
	printf("End of program\n");
	return 0;
}