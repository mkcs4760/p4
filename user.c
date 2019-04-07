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

//25% chance of returning 0, 75% chance of returning 1
int randomNum() {
	int num = rand() % 4;
	if (num == 0)
		return 0;
	else	
		return 1;
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
			printf("Child %d Received message : %s \n", getpid(), message.mesg_text); 
		}
		
		int returnTo = message.return_address;
		//now send a response
		
		message.mesg_type = returnTo; //send a message to this specific child, and no other
		
		int choice = randomNum();// get a random choice. This will be modified later, don't worry about it for now
		if (choice == 0) {
			//let's assume for now we are done
			//deallocate resources and continue
			printf("Process %d is finished.\n", getpid());
			strncpy(message.mesg_text, "done", 100);
			message.mesg_value = 0; //0 means done
			terminate = 1;
		} else {
			//let's assume for now we are not done
			//NOTE, LATER ON WE'LL HAVE TO DISTINGUISH BETWEEN BLOCKED AND NOT BLOCKED!!!!!
			printf("Process %d is not complete!\n", getpid());
			strncpy(message.mesg_text, "notDone", 100);
			message.mesg_value = 1; //0 means done
		}	
		
		message.return_address = getpid(); //tell them who sent it
		// msgsnd to send message 
		printf("Process %d sends message and gives control back to parent process\n", getpid());
		int send = msgsnd(msgid, &message, sizeof(message), 0);
		if (send == -1) {
			perror("Error on msgsnd\n");
		}

	}
		
	
	printf("ending child process %d\n", getpid());
	
	return 0;
}