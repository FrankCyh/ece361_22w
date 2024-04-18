#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "message.h"

#define STDIN 0

char serverAddress[100];
char serverPortNum[100];
int loggedIn  = 0;
int inSession = 0;


int inSession = 0;


int checkCommand(char *cmd) {  //% convert from user command to MACRO
    if (strcmp(cmd, "/login") == 0) {
        return LOGIN;
    } else if (strcmp(cmd, "/logout") == 0) {
        return EXIT;
    } else if (strcmp(cmd, "/joinsession") == 0) {
        return JOIN;
    } else if (strcmp(cmd, "/leavesession") == 0) {
        return LEAVE_SESS;
    } else if (strcmp(cmd, "/createsession") == 0) {
        return NEW_SESS;
    } else if (strcmp(cmd, "/list") == 0) {
        return QUERY;
    } else if (strcmp(cmd, "/quit") == 0) {
        return -1;
    } else {
        return MESSAGE;
    }
}

// return 0 on success
int constructMessage(int command, char *theFirst, char *theRest, struct Message *msg) {
    //% command is the MACRO

    //% return value:
    //% 0 is normal message
    //% 1 is for incomplete command
    //% 2 is for invalid command


    //# LOGIN
    if (command == LOGIN) {
        msg->type  = LOGIN;
        char *id   = strtok(NULL, " ");
        char *pwd  = strtok(NULL, " ");
        char *ip   = strtok(NULL, " ");
        char *port = strtok(NULL, " ");
        if (id == NULL || pwd == NULL || ip == NULL || port == NULL) {
            fprintf(stderr, "usage: /login <client-ID> <password> <server-IP> <server-port>\n");
            return 1;  //% 1 is for not complete command
        }
        strcpy(msg->source, id);
        strcpy(serverAddr, ip);
        strcpy(portNum, port);
        msg->size = strlen(pwd);
        strcpy(msg->data, pwd);  //% use the data field for the password

        //# EXIT
    } else if (command == EXIT || (loggedIn && command == -1)) {  // exit server
        msg->type = EXIT;
        msg->size = 0;
        strcpy(msg->data, "");  //% do not need message data

    } else if (command == JOIN) {
        msg->type = JOIN;
        if (theRest == NULL) {
            return 1;  //% incomplete
        } else {
            msg->size = strlen(theRest);
            strcpy(msg->data, theRest);
        }

        dataLen       = strlen(theRest);
        message->size = dataLen;
        strcpy(message->data, theRest);

    } else if (command == NEW_SESS && !inSession) {
        message->type = NEW_SESS;
        if (theRest == NULL) {
            return 1;
        } else {
            msg->size = strlen(theRest);
            strcpy(msg->data, theRest);
        }
        dataLen       = strlen(theRest);
        message->size = dataLen;
        strcpy(message->data, theRest);

    } else if (command == LEAVE_SESS) {
        msg->type = LEAVE_SESS;
        msg->size = 0;
        strcpy(msg->data, "");
        inSession = 0;

    } else if (command == QUERY) {
        msg->type = QUERY;
        msg->size = 0;
        strcpy(msg->data, "");

    } else if (command == -1) {
        msg->type = -1;

    } else if (command == MESSAGE && inSession) {  //% send message, combine two parts
        msg->type = MESSAGE;
        msg->size = strlen(theFirst) + 1 + strlen(theRest);
        sprintf(msg->data, "%s %s", theFirst, theRest);

    } else {
        return 2;  //% not in any of the case above, invalid command
    }
    return 0;
}

void printAckAndUpdateSession(struct Message *msg) {
    switch (msg->type) {
    case LO_ACK:
        printf("login successful\n");
        break;
    case LO_NAK:
        printf("login failed: %s\n", msg->data);
        break;
    case JN_ACK:
        printf("join session successful\n");
        inSession = 1;
        break;
    case JN_NAK:
        printf("join session failed: %s\n", msg->data);
        break;
    case NS_ACK:
        printf("create session successful\n");
        inSession = 1;
        break;
    case NS_NAK:
        printf("session alreaday exists\n");
        inSession = 1;
        break;
    case QU_ACK:
        printf("#User_id# : #Session_id#\n%s", msg->data);
        break;/
    case MESSAGE:
        break;
    default:
        printf("DONT KNOW\n");
        break;
    }

    // if (msg->type == LO_ACK) {
    //     printf("We IN\n");
    // } else if (msg->type == LO_NAK) {
    //     printf("%s\n", msg->data);
    // } else if (msg->type == JN_ACK) {
    //     printf("We Joined\n");
    //     inSession = 1;
    // } else if (msg->type == JN_NAK) {
    //     printf("%s\n", msg->data);
    // } else if (msg->type == NS_ACK) {
    //     printf("We Created a Sesh\n");
    //     inSession = 1;
    // } else if (msg->type == QU_ACK) {
    //     printf("Session List: \n%s", msg->data);
    // }

    // deleted from previous year
    // } else if (msg->type == NS_NAK) {
    //     printf("%s\n", msg->data);

    // deleted from previous year
    // else if (msg->type == LS_ACK) {
    //     printf("We Outta Session\n");
    //     inSession = 0;
    // }
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "usage: client");
        exit(1);
    }

    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int socketfd;
    int rv;  //% error variable
    int num_bytes_sent = 0, num_bytes_recv = 0;  //% used for communicate message length

    fd_set read_fds;

    char buffer[1000];  //% the char array to receive is of size 1000

    int quit = 0;

    while (!quit) {
        //int err;
        struct Message message;
        char inputPre[500];  //% the char array to send is of size 500

        //# In session
        if (inSession) {  //% if in session, have to receive message from the same session
            /// printf("in session\n");///

            //$ Preparation
            int fd_max = STDIN;

            /* Set the bits for the file descriptors you want to wait on. */
            FD_ZERO(&read_fds);
            FD_SET(STDIN, &read_fds);  //% wait on stdio
            FD_SET(socketfd, &read_fds);  //% wait for new connection request

            if (socketfd > fd_max) {
                fd_max = socketfd;  //% if the socketfd is larger than the fd_max right now, update the fd_max
            }

            if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {  //% on error, -1 is returned, on timeout, 0 is returned
                perror("select:");
                exit(1);
            }

            //$ 1. receive message from the session
            if (FD_ISSET(socketfd, &read_fds)) {  //% the client's socket is updated
                /* There is data waiting on your socket.  Read it with recv(). */
                num_bytes_recv = recv(socketfd, (char *)buffer, 1000, 0);
                if (num_bytes_recv == -1) {  //% -1 indicates error
                    perror("client: recv1");
                    exit(1);
                }
                buffer[num_bytes_recv] = '\0';  //% set the last bit of the received char array to a null ptr
                struct Message recvMsg;  //% struct Message for received message

                parse_message(buffer, &recvMsg);
                printf("From Conf Session: %s\n", recvMsg.data);
            }

            //$ 2. receive message or command from the user's STDIN
            //% will have to differentiate message from command
            if (FD_ISSET(STDIN, &read_fds)) {
                /* The user typed something.  Read it fgets or something.
                Then send the data to the server. */
                fgets(inputPre, sizeof(inputPre), stdin);

                //$ 2.1 Check if the command have error
                char *inputPost = strtok(inputPre, "\n");

                char *firstWord = strtok(inputPost, " ");  //% firstword is the command

                inputPost = inputPost + strlen(inputPost) + 1;  // to find theRest

                int command = checkCommand(firstWord);  //% get the MACRO

                if (command == -1 && !loggedIn) {  //% trying to quit but not logged in yet
                    break;

                } else if (command == -1 && loggedIn) {  //% logged in, haven't join a session, thus can quit in the next round
                    quit = 1;
                }


                //err = constructMessage(command, firstWord, inputPost, &message);  //% get the error type
                if (command == LOGIN) {
                    message.type = LOGIN;
                    char *id = strtok(NULL, " ");
                    char *pwd = strtok(NULL, " ");
                    char *ip = strtok(NULL, " ");
                    char *port = strtok(NULL, " ");
                    if (id == NULL || pwd == NULL || ip == NULL || port == NULL) {
                        fprintf(stderr, "usage: /login <client-ID> <password> <server-IP> <server-port>\n");
                        //err = 1; //% 1 is for not complete command
                        printf("INCOMPLETE COMMAND\n");
                        continue;
                    }
                    strcpy(message.source, id);
                    strcpy(serverAddr, ip);
                    strcpy(portNum, port);
                    message.size = strlen(pwd);
                    strcpy(message.data, pwd); //% use the data field for the password

                    //# EXIT
                } else if (command == EXIT || (loggedIn && command == -1)) { // exit server
                    message.type = EXIT;
                    message.size = 0;
                    strcpy(message.data, ""); //% do not need message data

                } else if (command == JOIN) {
                    message.type = JOIN;
                    if (inputPost == NULL) {
                        //err = 1; //% incomplete
                        printf("INCOMPLETE COMMAND\n");
                        continue;
                    } else {
                        message.size = strlen(inputPost);
                        strcpy(message.data, inputPost);
                    }

                } else if (command == NEW_SESS && !inSession) {
                    message.type = NEW_SESS;
                    if (inputPost == NULL) {
                        //err = 1;
                        printf("INCOMPLETE COMMAND\n");
                        continue;
                    } else {
                        message.size = strlen(inputPost);
                        strcpy(message.data, inputPost);
                    }

                } else if (command == LEAVE_SESS) {
                    message.type = LEAVE_SESS;
                    message.size = 0;
                    strcpy(message.data, "");
                    inSession = 0;

                } else if (command == QUERY) {
                    message.type = QUERY;
                    message.size = 0;
                    strcpy(message.data, "");
                
                } else if (command == -1) {
                    message.type = -1;

                } else if (command == MESSAGE && inSession) { //% send message, combine two parts
                    message.type = MESSAGE;
                    message.size = strlen(firstWord) + 1 + strlen(inputPost);
                    sprintf(message.data, "%s %s", firstWord, inputPost);

                } else {
                    //err = 2; //% not in any of the case above, invalid command
                    printf("CANNOT SEND A MESSAGE OUT OF SESSION\n");
                    continue;
                }

                //% Report the error; This can be put in the parse_message section
                /*if (err) {
                    if (err == 1) {
                        printf("INCOMPLETE COMMAND\n");
                    } else if (err == 2) {
                        printf("CANNOT SEND A MESSAGE OUT OF SESSION\n");
                    }

                    continue;  //% not proceed any further
                }*/

                //$ 2.2 login for anther user
                //% The current user is already logged in, thus this is for another user
                if (!loggedIn && command == LOGIN) {
                    int rv = 0;
                    if ((rv = getaddrinfo(serverAddr, portNum, &hints, &servinfo)) != 0) {
                        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                        return 1;
                    }

                    //% login: create socket and bind
                    socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
                    if (socketfd < 0) {
                        perror("Error while creating socket\n");
                        return -1;
                    }
                    // printf("Socket created successfully\n");

                    if (connect(socketfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                        close(socketfd);
                        perror("Error connecting to socket\n");
                    }
                    // printf("Connection successful\n");
                    loggedIn = 1;
                }

                //$ 2.3 not command, is a message, convert the message to send
                char messageString[MAX_OVER_NETWORK];
                memset(messageString, 0, sizeof(messageString));
                sprintf(messageString, "%d:%d:%s:%s", message.type, message.size, message.source, message.data);

                //% send to the server
                num_bytes_sent = send(socketfd, messageString, strlen(messageString), 0);
                if (num_bytes_sent == -1) {
                    perror("client: send1");
                    exit(1);
                }
                // printf("Sent: %s\n", messageString);

                //% tell the server I am going to exit
                if (loggedIn && command == EXIT) {
                    freeaddrinfo(servinfo);
                    close(socketfd);
                    loggedIn  = 0;
                    inSession = 0;
                    continue;
                }

                //char buffer[1000];

                //% get the acknowledgement
                if (command != LEAVE_SESS) {
                    num_bytes_recv = recv(socketfd, (char *)buffer, 1000, 0);
                    if (num_bytes_recv == -1) {
                        perror("client: recv1");
                        exit(1);
                    }
                    buffer[num_bytes_recv] = '\0';
                    // printf("Received: %s\n", buffer);
                }

                struct Message recvMsg;
                parse_message(buffer, &recvMsg);

                if (recvMsg.type == LO_NAK) {  // would i close something already closed??
                    freeaddrinfo(servinfo);
                    close(socketfd);
                    loggedIn = 0;
                }

                showACK(&recvMsg);
            }

            //# Currently out of session
        } else {
            /// printf("out of session\n");///
            fgets(inputPre, sizeof(inputPre), stdin);
            char *inputPost = strtok(inputPre, "\n");
            char *firstWord = strtok(inputPost, " ");
            inputPost       = inputPost + strlen(inputPost) + 1;  // to find theRest

            int command = checkCommand(firstWord);  //% convert from user lowercase command to uppercase MACRO

            if (command == -1 && !loggedIn) {  //% trying to quit but not logged in yet
                break;
            } else if (command == -1 && loggedIn) {  //% logged in, haven't join a session, thus can quit in the next round
                quit = 1;
            }


            //err = constructMessage(command, firstWord, inputPost, &message);  //% get the error type
            if (command == LOGIN) {
                message.type = LOGIN;
                char *id = strtok(NULL, " ");
                char *pwd = strtok(NULL, " ");
                char *ip = strtok(NULL, " ");
                char *port = strtok(NULL, " ");
                if (id == NULL || pwd == NULL || ip == NULL || port == NULL) {
                    fprintf(stderr, "usage: /login <client-ID> <password> <server-IP> <server-port>\n");
                    //err = 1; //% 1 is for not complete command
                    printf("INCOMPLETE COMMAND\n");
                    continue;
                }
                strcpy(message.source, id);
                strcpy(serverAddr, ip);
                strcpy(portNum, port);
                message.size = strlen(pwd);
                strcpy(message.data, pwd); //% use the data field for the password

                //# EXIT
            } else if (command == EXIT || (loggedIn && command == -1)) { // exit server
                message.type = EXIT;
                message.size = 0;
                strcpy(message.data, ""); //% do not need message data

            } else if (command == JOIN) {
                message.type = JOIN;
                if (inputPost == NULL) {
                    //err = 1; //% incomplete
                    printf("INCOMPLETE COMMAND\n");
                    continue;
                } else {
                    message.size = strlen(inputPost);
                    strcpy(message.data, inputPost);
                }

            } else if (command == NEW_SESS && !inSession) {
                message.type = NEW_SESS;
                if (inputPost == NULL) {
                    //err = 1;
                    printf("INCOMPLETE COMMAND\n");
                    continue;
                } else {
                    message.size = strlen(inputPost);
                    strcpy(message.data, inputPost);
                }

            } else if (command == LEAVE_SESS) {
                message.type = LEAVE_SESS;
                message.size = 0;
                strcpy(message.data, "");
                inSession = 0;

            } else if (command == QUERY) {
                message.type = QUERY;
                message.size = 0;
                strcpy(message.data, "");
                
            } else if (command == -1) {
                message.type = -1;

            } else if (command == MESSAGE && inSession) { //% send message, combine two parts
                message.type = MESSAGE;
                message.size = strlen(firstWord) + 1 + strlen(inputPost);
                sprintf(message.data, "%s %s", firstWord, inputPost);

            } else {
                //err = 2; //% not in any of the case above, invalid command
                printf("CANNOT SEND A MESSAGE OUT OF SESSION\n");
                continue;
            }

            //% Report the error; This can be put in the parse_message section
            /*if (err) {
                if (err == 1) {
                    printf("INCOMPLETE COMMAND\n");
                } else if (err == 2) {
                    printf("CANNOT SEND A MESSAGE OUT OF SESSION\n");
                }

                continue;  //% not proceed any further
            }*/

            //----------------------------------------------------------------------------------------------------------

            //$ LOGIN
            if (!loggedIn && command == LOGIN) {
                int rv = 0;
                if ((rv = getaddrinfo(serverAddr, portNum, &hints, &servinfo)) != 0) {
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                    return 1;
                }

                socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
                if (socketfd < 0) {
                    perror("Error while creating socket\n");
                    return -1;
                }
                // printf("Socket created successfully\n");

                if (connect(socketfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                    close(socketfd);
                    perror("Error connecting to socket\n");
                }
                // printf("Connection successful\n");
                loggedIn = 1;
            }


            char messageString[MAX_OVER_NETWORK];
            memset(messageString, 0, sizeof(messageString));
            sprintf(messageString, "%d:%d:%s:%s", message.type, message.size, message.source, message.data);


            //$ Send to server
            num_bytes_sent = send(socketfd, messageString, strlen(messageString), 0);
            if (num_bytes_sent == -1) {
                perror("client: send2");
                exit(1);
            }
            // printf("Sent: %s\n", messageString);


            //$ EXIT
            if (loggedIn && command == EXIT) {
                freeaddrinfo(servinfo);
                close(socketfd);
                loggedIn  = 0;
                inSession = 0;
                continue;
            }

            //$ receive from server
            if (command != LEAVE_SESS) {
                num_bytes_recv = recv(socketfd, (char *)buffer, 1000, 0);
                if (num_bytes_recv == -1) {
                    perror("client: recv1");
                    exit(1);
                }
                buffer[num_bytes_recv] = '\0';
                // printf("Received: %s\n", buffer);
            }

            struct Message recvMsg;
            parse_message(buffer, &recvMsg);

            if (recvMsg.type == LO_NAK) {  // would i close something already closed??
                freeaddrinfo(servinfo);
                close(socketfd);
                loggedIn = 0;
            }

            showACK(&recvMsg);
        }
    }


    return 0;
}
