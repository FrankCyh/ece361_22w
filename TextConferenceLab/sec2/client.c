#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "message.h"

char serverAddress[100];
char serverPortNum[100];
bool loggedIn  = false;
int numSession = 0;  // record the number of session this user is currently in

// return true on success and false on failure
bool create_message(int type, char *command, char *arguments, struct Message *message) {
    // command is for PRIVATE and MESSAGE
    // commmand /quit is handle in the while loop in main
    int num_bytes = 0;

    // /login <client ID> <password> <server-IP> <server-port>
    if (type == LOGIN) {
        char *client_ID = strtok(NULL, " ");
        char *password  = strtok(NULL, " ");
        char *servIP    = strtok(NULL, " ");
        char *servPort  = strtok(NULL, " ");
        char *left      = strtok(NULL, " ");
        if (client_ID == NULL || password == NULL || servIP == NULL || servPort == NULL || left != NULL) {
            printf("Error: incomplte command\n");
            printf("Use /login <client ID> <password> <server-IP> <server-port>\n\n");
            return false;
        }
        strcpy(serverAddress, servIP);
        strcpy(serverPortNum, servPort);

        message->type = LOGIN;
        message->size = strlen(password);
        strcpy(message->source, client_ID);  // source only need to configure once
        strcpy(message->data, password);

    } else if (type == LOGOUT || type == QUIT) {
        // for QUIT, have to do LOGOUT otherwise segmentation fault
        message->type = LOGOUT;
        message->size = 0;
        // source is set at login
        strcpy(message->data, "");


    } else if (type == JOIN && loggedIn) {
        if (arguments == NULL) {
            printf("Error: incomplte command\n");
            printf("Use /joinsession <session ID>\n\n");
            return false;
        }
        message->type = JOIN;
        num_bytes     = strlen(arguments);
        message->size = num_bytes;
        strcpy(message->data, arguments);

    } else if (type == NEW_SESS && loggedIn) {
        if (arguments == NULL) {
            printf("Error: incomplte command\n");
            printf("Use /createsession <session ID>\n\n");
            return false;
        }
        message->type = NEW_SESS;
        num_bytes     = strlen(arguments);
        message->size = num_bytes;
        strcpy(message->data, arguments);

    } else if (type == LEAVE_SESS && loggedIn) {
        if (arguments == NULL) {
            printf("Error: incomplte command\n\n");
            printf("Use /leavesession <session ID>\n\n");
            return false;
        }
        message->type = LEAVE_SESS;
        num_bytes     = strlen(arguments);
        message->size = num_bytes;
        strcpy(message->data, arguments);

    } else if (type == QUERY && loggedIn) {
        message->type = QUERY;
        message->size = 0;
        strcpy(message->data, "");

    } else if (type == PRIVATE && loggedIn) {
        if (strstr(command, ":") == NULL) {
            printf("Error: To send a private message, use :<user_to_send> <message>\n\n");
            return false;
        }
        message->type = PRIVATE;
        num_bytes     = sprintf(message->data, "%s %s", command + 1, arguments);
        message->size = num_bytes;

    } else if (numSession > 0) {
        message->type = MESSAGE;
        if (strstr(command, "/") == NULL) {
            printf("Error: To send a message to session, use /<session_id> <message>\n\n");
            return false;
        }
        num_bytes     = sprintf(message->data, "%s %s", command + 1, arguments);
        message->size = num_bytes;  // excluding NULL character

    } else {
        // list of available commands
        printf("Error: Invalid command\n");
        printf("Choose from one of the following commands: \n");
        printf("----------------------------------------------------------\n");
        printf("  1. /login <client ID> <password> <server-IP> <server-port> \n");
        printf("  2. /logout \n");
        printf("  3. /quit \n");
        printf("  4. /joinsession <session ID>\n");
        printf("  5. /leavesession <session ID>\n");
        printf("  6. /createsession <session ID>\n");
        printf("  7. /<joined_session_id> <message>\n");
        printf("  8. /list \n");
        printf("  p.s. command 4 - 8 should be request after login \n");
        printf("----------------------------------------------------------\n");
        if (!loggedIn)
            printf("You are currently not logged in.\n");
        else
            printf("You currently have %d session.\n", numSession);
        printf("----------------------------------------------------------\n\n");
        return false;
    }

    return true;
}


int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "usage: client");
        exit(1);
    }

    // global and loop variables
    loggedIn   = false;
    numSession = 0;
    bool quit  = false;

    int sockfd = -1;
    fd_set read_fds;
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv             = 0;  // error variable
    int num_bytes_sent = 0, num_bytes_rcv = 0;

    char rcv_buffer[MAX_MESSAGE];

    while (!quit) {
        struct Message rcv_struct_msg;
        struct Message sent_struct_msg;

        //# Preparation
        int fd_max = STDIN_FILENO;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        if (sockfd != -1)  // connection already established
            FD_SET(sockfd, &read_fds);
        if (sockfd > fd_max)  // for log out and log in again ?
            fd_max = sockfd;

        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {  // returned -1 on error , return 0 on timeout
            perror("client: select");
            exit(1);
        }

        //# 1. Receive message from the session
        if (FD_ISSET(sockfd, &read_fds)) {
            if ((num_bytes_rcv = recv(sockfd, (char *)rcv_buffer, MAX_MESSAGE, 0)) == -1) {
                perror("client: recv");
                exit(1);
            }
            rcv_buffer[num_bytes_rcv] = '\0';

            print_msg(rcv_buffer, false);
            parse_message(rcv_buffer, &rcv_struct_msg);

            // Session message received or private message received
            if (rcv_struct_msg.type == MESSAGE)
                printf("Session %s: %s\n\n", rcv_struct_msg.source, rcv_struct_msg.data);
            else if (rcv_struct_msg.type == PRIVATE)
                printf("User %s: %s\n\n", rcv_struct_msg.source, rcv_struct_msg.data);
        }


        //# 2. Receive message or command from the user's STDIN
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char usr_input[MAX_MESSAGE];
            fgets(usr_input, sizeof(usr_input), stdin);

            //$ 2.1 Get the command MACRO and command content
            char *input_line = strtok(usr_input, "\n");
            if (input_line == NULL)
                continue;  // otherwise will cause segmentation fault
            char *first_word = strtok(input_line, " ");  // firstword is the command
            input_line       = input_line + strlen(input_line) + 1;
            int command      = get_MACRO(first_word);

            //$ create Message is valid or not
            bool valid = create_message(command, first_word, input_line, &sent_struct_msg);
            if (!valid)
                continue;  // skip the rest of this round

            //* Handle two cases of LOGOUT and QUIT manually
            // 1. Quiting the program: skip all execution afterwards
            if (command == QUIT) {  //* still have to send message to set the state to inactive
                quit = true;
                printf("Quiting the program ... \n\n");
                if (!loggedIn)
                    continue;  // already logged out, no need to send message
            }
            // 2. Trying to logout but not logged in yet
            else if (command == LOGOUT && !loggedIn) {
                printf("Error: Trying to logout but not logged in yet.\n");
                continue;
            }

            //$ User LOGIN, establish connection
            // reserve getaddrinfo and process after receive feedback from server
            if (command == LOGIN) {
                if (!loggedIn) {
                    if ((rv = getaddrinfo(serverAddress, serverPortNum, &hints, &servinfo)) != 0) {
                        perror("Client: getaddrinfo");
                        exit(1);
                    }

                    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
                    if (sockfd < 0) {
                        perror("Client: socket");
                        exit(1);
                    }

                    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                        close(sockfd);
                        sockfd = -1;
                        perror("Client: connect");
                    }
                } else {
                    printf("Error: User already logged in. Please /logout frist. \n\n");
                    continue;
                }
            }


            //$ Send to server
            char sent_msg[MAX_MESSAGE];
            memset(sent_msg, 0, sizeof(sent_msg));
            sprintf(sent_msg, "%d:%d:%s:%s", sent_struct_msg.type, sent_struct_msg.size, sent_struct_msg.source, sent_struct_msg.data);

            if ((num_bytes_sent = send(sockfd, sent_msg, strlen(sent_msg), 0) == -1)) {
                perror("Client: send");
                exit(1);
            }
            print_msg(sent_msg, true);  // print the message sent


            //$ User LOGOUT
            // Must first send the message then log out, because we still need sockfd to send message
            if (loggedIn && (command == LOGOUT || command == QUIT)) {
                freeaddrinfo(servinfo);
                close(sockfd);
                sockfd     = -1;
                loggedIn   = false;
                numSession = 0;
                printf("\n");
                continue;  // no ACK for LOGOUT, do not need to listen anymore
            }

            //$ Receive from server and check acknowledgement
            if ((num_bytes_rcv = recv(sockfd, (char *)rcv_buffer, MAX_MESSAGE, 0)) == -1) {
                perror("Client: recv");
                exit(1);
            }
            rcv_buffer[num_bytes_rcv] = '\0';

            print_msg(rcv_buffer, false);  // print the message received
            parse_message(rcv_buffer, &rcv_struct_msg);

            //$ Update state variables
            if (rcv_struct_msg.type == LO_ACK) {
                loggedIn = true;
            } else if (rcv_struct_msg.type == LO_NAK) {
                freeaddrinfo(servinfo);  // login failed, free the addrinfo reserved for this login
                close(sockfd);
                sockfd = -1;
            } else if (rcv_struct_msg.type == JN_ACK) {
                numSession += 1;
            } else if (rcv_struct_msg.type == NS_ACK) {
                numSession = +1;
            } else if (rcv_struct_msg.type == LS_ACK) {
                numSession -= 1;
            } else if (rcv_struct_msg.type == QU_ACK) {
                // print session list
                printf("----------------------------------------\n");
                printf("%s", rcv_struct_msg.data);
                printf("----------------------------------------\n");
            }

            printf("\n");
        }
    }

    return 0;
}
