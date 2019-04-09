#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <string.h>
#include <time.h>
#include "messageQueue.h"

/*int randomNum(int max) {
	int num = (rand() % (max + 1));
	return num;
}*/

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
	
	//printf("We are in user\n");
	//printf("Me child! My pid is %d\n", getpid());
	//first we set up the message queue
	
	key_t key; 
    int msgid; 
  
    key = 1094; 
  
    // msgget creates a message queue 
    // and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT); 
	
	int terminate = 0;
	
	while (!terminate) {
		
		// msgrcv to receive message 
		int receive;
		receive = msgrcv(msgid, &message, sizeof(message), getpid(), 0); 
		if (receive < 0) {
			perror("No message received\n");
		} else {
			//printf("Child %d Received message : %s \n", getpid(), message.mesg_text); 
		}
		
		int returnTo = message.return_address;
		//now send a response
		
		message.mesg_type = returnTo; //send a message to this specific child, and no other
		
		int choice = randomNum(4);// get a random choice. This will be modified later, don't worry about it for now
		if (choice == 0) {
			//let's assume for now we are done
			//deallocate resources and continue
			//printf("Process %d is finished.\n", getpid());
			//NOW WE NEED TO SEND BACK A VALUE THAT INDICATES WE ARE DONE AND HOW MUCH OF OUR TIMESHARE WE USED
			strncpy(message.mesg_text, "done", 100);
			//now let's decide how much of our time we used in order to finish our process
			int timeUsed = randomTimeUsed();
			//message.mesg_value = 0; //0 means done
			message.mesg_value = timeUsed;
			terminate = 1;
		} else {
			//let's assume for now we are not done
			//NOTE, LATER ON WE'LL HAVE TO DISTINGUISH BETWEEN BLOCKED AND NOT BLOCKED!!!!!
			//printf("Process %d is not complete!\n", getpid());
			strncpy(message.mesg_text, "notDone", 100);
			//message.mesg_value = 1; //0 means done
			int choice2 = randomNum(2);
			if (choice2 == 0) {
				//we used all of our timeslice
				
				//NOW WE NEED TO SEND BACK A VALUE THAT INDICATES WE ARE NOT DONE AND WE USED ALL OF OUR TIMESHARE
				message.mesg_value = 0; // 0 means not done, but we used 100% of our time
				//pause the process
				//return control to parent
				//place the process in the back of the proper queue
				//increment clock by the timeslice we were given
			} else {
				//we did not use all of our timeslice and are not blocked
				
				//NOW WE NEED TO SEND BACK A VALUE THAT INDICATES WE ARE NOT DONE AND WE WERE BLOCKED PARTWAY THROUGH OUR TIMESHARE
				int timeUsed = randomTimeUsed();
				message.mesg_value = (timeUsed * -1); //a negative value means we did not finish and we were blocked after using this portion of out timeslice
				
				//randomly determine a percentage of our slice that was used
				//send said value to parent
				//have parent increment clock by said amount
				//place this process in a blocked queue
				//randomly determine how long the process should be blocked, and connect that value to the process
			}
		}	
		
		message.return_address = getpid(); //tell them who sent it
		// msgsnd to send message 
		//printf("Process %d sends message %d and gives control back to parent process\n", getpid(), message.mesg_value);
		int send = msgsnd(msgid, &message, sizeof(message), 0);
		if (send == -1) {
			perror("Error on msgsnd\n");
		}

	}
		
	
	//printf("ending child process %d\n", getpid());
	
	return 0;
}