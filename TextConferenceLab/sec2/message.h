#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MAX_NAME 25
#define MAX_DATA 500
#define MAX_MESSAGE 600

struct Message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

enum MessageType { LOGIN,  // $login
                   LO_ACK,
                   LO_NAK,
                   LOGOUT,  // $logout
                   // replace EXIT with logout to avoid ambiguity
                   JOIN,  // $join
                   JN_ACK,
                   JN_NAK,
                   LEAVE_SESS,  // $leavesession
                   LS_ACK,  // added for acknowledgement
                   LS_NAK,  // added for negative acknowledgement
                   NEW_SESS,  // $createsession
                   NS_ACK,
                   NS_NAK,  //* added for negative acknowledgement
                   QUERY,  //$ list
                   QU_ACK,  // no negative acknowledgement for QUERY
                   MESSAGE,  //$ message
                   MESSAGE_ACK,  // added for acknowledgement
                   MESSAGE_NAK,  // added for negative acknowledgement
                   PRIVATE,  // $ private message
                   PRIVATE_ACK,  // added for acknowledgement
                   PRIVATE_NAK,  // added for negative acknowledgement
                   QUIT  //$ quit
                   };

void parse_message(char* str, struct Message* msg);

void print_msg(char* msg, bool sent);

void print_MACRO(int num);

int get_MACRO(char* command);

#endif /* _MESSAGE_H_ */
