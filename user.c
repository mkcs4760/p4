#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <string.h>
#include "messageQueue.h"

int main(int argc, char *argv[]) {
	
	//printf("We are in user\n");
	//printf("Me child! My pid is %d\n", getpid());
	//first we set up the message queue
	
	key_t key; 
    int msgid; 
  
    key = 1094; 
  
    // msgget creates a message queue 
    // and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT); 
	
	// msgrcv to receive message 
    int receive;
	receive = msgrcv(msgid, &message, sizeof(message), getpid(), 0); 
	if (receive < 0) {
		perror("No message received\n");
	} else {
		printf("Child Received message : %s \n", message.mesg_text); 
		printf("Yay! This is the child and now I am allowed to run!\n");
	}
	int returnTo = message.return_address;
	//now send a response
	
	message.mesg_type = returnTo; //send a message to this specific child, and no other
	//message.mesg_text = "doneHere"; 
	strncpy(message.mesg_text, "doneHere", 100);
	message.return_address = getpid(); //tell them who sent it
	// msgsnd to send message 
	printf("But now I am done, so I give control back to parent process\n");
	int send = msgsnd(msgid, &message, sizeof(message), 0);
	if (send == -1) {
		perror("Error on msgsnd\n");
	}
	printf("ending child process\n");
	
	return 0;
}