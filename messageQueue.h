#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

// structure for message queue 
struct mesg_buffer { 
    long mesg_type; 
    char mesg_text[100];
	int mesg_value;
	int return_address;
} message; 

#endif