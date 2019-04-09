#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <string.h>
#include <time.h>
#include "messageQueue.h"

//if 4 passed in, 25% chance of returning 0, 75% chance of returning 1
//if 2 passed in, 50% chance of returning 0, 50% chance of returning 1
int randomNum(int num) {
	int newNum = rand() % num;
	if (newNum == 0)
		return 0;
	else	
		return 1;
}

//returns a random number between 1 and 9
//this represents what percentage (10% to 90%) of the time was used
int randomTimeUsed() {
    int num = (rand() % 9) + 1;
    //printf("%d ", num);
    return num;	
}

int main(int argc, char *argv[]) {
	srand(time(0)); //placed here so we can generate random numbers later on
	
	//first we set up the message queue
	
	key_t key; 
    int msgid; 
  
    key = 1094; 
  
    //msgget creates a message queue  and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT); 
	
	int terminate = 0;
	
	while (!terminate) { //this while loop repeats until the process "decides" it is done
		// msgrcv to receive message 
		int receive;
		receive = msgrcv(msgid, &message, sizeof(message), getpid(), 0); 
		if (receive < 0) {
			perror("No message received\n");
		}
		
		int returnTo = message.return_address;
		//now send a response
		
		message.mesg_type = returnTo; //send a message to this specific child, and no other
		
		int choice = randomNum(4); //get a random choice. This will be modified later, don't worry about it for now
		if (choice == 0) { //process is complete, deallocate resources and continue
			strncpy(message.mesg_text, "done", 100);
			int timeUsed = randomTimeUsed(); //the amount of time we used
			message.mesg_value = timeUsed;
			terminate = 1;
		} else { //process is not complete...
			strncpy(message.mesg_text, "notDone", 100);
			int choice2 = randomNum(2); //...but did we get blocked or not?
			if (choice2 == 0) { //we did not get blocked, we used all of our timeslice
				message.mesg_value = 0; // 0 means not done, but we used 100% of our time
			} else { //we did get blocked
				int timeUsed = randomTimeUsed();
				message.mesg_value = (timeUsed * -1); //a negative value means we did not finish and we were blocked after using this portion of out timeslice
			}
		}	
		
		message.return_address = getpid(); //tell them who sent it
		int send = msgsnd(msgid, &message, sizeof(message), 0); //send message back to parent
		if (send == -1) {
			perror("Error on msgsnd\n");
		}
	}

	return 0;
}