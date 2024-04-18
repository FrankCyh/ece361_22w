#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MAX_NAME 25
#define MAX_DATA 500
#define MAX_GLOBAL 600

struct Message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

enum MessageType { LOGIN,  // login
                   LO_ACK,
                   LO_NAK,
                   EXIT,  // quit
                   JOIN,  // join
                   JN_ACK,
                   JN_NAK,
                   LEAVE_SESS,  // leavesession; no need for ACK
                   NEW_SESS,  // createsession
                   NS_ACK,
                //    NS_NAK,  // added for negative acknowledgement
                   MESSAGE, // <text>
                   QUERY, // list
                   QU_ACK };

void parse_message(char* str, struct Message* msg);

#endif /* _MESSAGE_H_ */
