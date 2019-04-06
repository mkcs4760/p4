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

#define CLOCK_INC 100000000 //sample number, this is a constent. Experiment with it
#define MAX_TIME_BETWEEN_NEW_PROCESSES_NS 999999999
#define MAX_TIME_BETWEEN_NEW_PROCESSES_SECS 1 //we will create a new user process at a random interval between 0 and these values


struct PCB {
	//char data[100][80];
	int myPID; //your local simulated pid
	int timeCreatedSecs;
	int timeCreatedNano;
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

int main(int argc, char *argv[]) {
	printf("Welcome to project 4\n");
	srand(time(0)); //placed here so we can generate random numbers later on
	
	//we need a clock
	int clockSeconds = 0;
	int clockNano = 0;
	int clockInc = CLOCK_INC; 
	
	//we need our process control table
	int maxKidsAtATime = 1; //will be set by the "S" command line argument. Default should probably be 18, but for us we are using 1 for early testing...
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum blocks
	
	//our "bit vector" (or boolean array here) that will tell us which PRB are free
	bool boolArray[maxKidsAtATime]; //by default, these are all set to false. This has been tested and verified
	int i;
	for (i = 0; i < sizeof(boolArray); i++) {
		printf("Is PCB #%d being used? %d \n", i, boolArray[i]);
	}
	
	//set up our process queue
	struct Queue* queue = createQueue(maxKidsAtATime); //note: queue is the name of the queue. It is of type Queue, qhich is defined in queue.h. Pass in the max size
	
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
	
	for (i = 0; i < 50; i++) {
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
					//also actually launch the child
					pid_t pid;
					pid = fork();
					
					if (pid == 0) { //child
						execl ("user", "user", NULL);
						perror("execl failed. \n");
					}
					else if (pid > 0) { //parent
						printf("Created child %d at %d:%d\n", pid, clockSeconds, clockNano);
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
		//increment clock
		clockNano += clockInc;
		if (clockNano >= 1000000000) { //increment the next unit
			clockSeconds += 1;
			clockNano -= 1000000000;
		}
		//check to see if we should start a process
		if (!isEmpty(queue)) {
			//queue is not empty. Thus we should run something
			int nextPID = dequeue(queue); //decide the next process to run. Here it's simple with only one queue. Later this will require more attention
			
			//let's send this process a message
			message.mesg_type = nextPID; //send a message to this specific child, and no other
			strncpy(message.mesg_text, "go", 100); //right now we're just sending "go" later on we'll have to be more specific
			message.return_address = getpid(); //tell them who sent it
			// msgsnd to send message 
			printf("child will do nothing until we let it\n");
			printf("sending message for child to activate\n");
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
				printf("Data Received is : %s \n", message.mesg_text); //later on we'll have to process the response
				returnValue = message.mesg_value;
				printf("parent is now in control again\n");
			}
			//determine what to do with this pid. If it's done, do nothing. Else, send it back in.
			//int choice = randomNum(2); //note, the random stuff will have to happen in user, not here. But for testing today, let's do it here
			if (returnValue == 0) {
				//let's assume for now we are done
				//deallocate resources and continue
				printf("Process %d is finished\n", nextPID);
				//find out which boolArray position needs to be free
				int j;
				for (j = 0; j < sizeof(PCT); j++) {
					if (PCT[j].myPID == nextPID) {
						boolArray[j] = 0;
						printf("We are deallocating the value at boolArray[%d]\n", j);
						break;
					}
				}
			} else if (returnValue == 1){
				//let's assume for now we are not done
				printf("Process %d is not complete and is going back in the end of the queue\n", nextPID);
				enqueue(queue, nextPID);//add again to the back of the queue
			} else {
				perror("ERROR! Invalid return value!");
			}
		}
		//process return value and continue
	}
	printf("We leave at %d:%d\n", clockSeconds, clockNano);
	
	//close message queue
	msgctl(msgid, IPC_RMID, NULL); 
	printf("That concludes this portion of the in-development program\n");
	printf("To avoid warnings, let's use values: %d \n", PCT[0].myPID);
	printf("End of program\n");
	return 0;
}