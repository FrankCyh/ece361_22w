#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 10  // Size of backlog queue
#define NUM_USERS 5

#include "message.h"

//# Database and Datastuctures

//$ Struct session
// use a linked list for session, all user id (char*) should be appended to the list
struct Session {
    char *name;
    struct Session *next;
};

//$ User database
// user id, password, state(active or not),
const char *users_list[]                     = {"Frank", "Nick", "Jack", "William", "Hamid"};
const char *passwords_list[]                 = {"frank", "nick", "jack", "william", "hamid"};
int users_active[NUM_USERS]                  = {0, 0, 0, 0, 0};  //% section active or not
struct Session *user_section_list[NUM_USERS] = {NULL, NULL, NULL, NULL, NULL};
int user_fds[NUM_USERS]                      = {-1, -1, -1, -1, -1};  //% once log in, update to each user's client_fd

struct User {
    char *name;
    char *password;
    int usr_fd;
    bool active;
    struct Session *session;
};

struct User usrs_list[NUM_USERS] = {
    {"Frank", "frank", -1, false, NULL},
    {"Nick", "nick", -1, false, NULL},
    {"Jack", "jack", -1, false, NULL},
    {"William", "william", -1, false, NULL},
    {"Hamid", "hamid", -1, false, NULL},
};

int getUsrIdx(unsigned char *usr_name) {
    for (int i = 0; i < NUM_USERS; i++)
        if (strcmp(usrs_list[i].name, usr_name) == 0)
            return i;
    return -1;
}

void command_handler(struct Message *msg, int client_fd) {
    int type = msg->type;

    char ack_msg[MAX_GLOBAL];
    memset(ack_msg, 0, sizeof(ack_msg));

    char *data;
    // memset(data, 0, sizeof(data)); // use this cause the segmentation fault

    char *source    = "Server";
    bool authorized = false;
    int i;

    char *client_id;
    strcpy(client_id, msg->source);

    int idx = getUsrIdx(client_id);
    if (idx == -1) {
        printf("Wrong user, no record in the database");
        return;
    } else
        printf("User %s detected \n", usrs_list[idx].name);

    //# LOGIN
    if (type == LOGIN) {
        printf("Command type: LOGIN\n");

        if (strcmp(usrs_list[idx].password, (char *)msg->data) == 0) {
            if (!usrs_list[idx].active) {
                // ACK: Not logged in yet
                usrs_list[idx].active = true;
                usrs_list[idx].usr_fd = client_fd;

                data = "";  // Data field should be empty
                sprintf(ack_msg, "%d:%d:%s:%s", LO_ACK, strlen(data), source, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
                printf("User %s log in successfully \n", usrs_list[idx].name);

            } else {
                // NACK: Already logged in
                data = "Error: Already logged in";
                sprintf(ack_msg, "%d:%d:%s:%s", LO_NAK, strlen(data), msg->source, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
            }
        } else {
            // NACK: Incorrect password
            data = "Error: Incorrect password for user";
            sprintf(ack_msg, "%d:%d:%s:%s", LO_NAK, strlen(data), source, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            close(client_fd);
        }


        //# EXIT
    } else if (type == EXIT) {
        printf("Command type: EXIT\n");
        usrs_list[idx].active  = false;
        usrs_list[idx].session = NULL;

        for (int i = 0; i < NUM_USERS; i++)
            if (strcmp(users_list[i], client_id) == 0) {
                users_active[i]      = 0;
                user_section_list[i] = NULL;  // the list created by the user should also be erased
            }
        //* if delete will cause segmentation fault
        printf("User %s log in successfully \n", usrs_list[idx].name);
        close(client_fd);


        //# JOIN
    } else if (type == JOIN) {
        printf("Command type: JOIN\n");
        char *session_name = msg->data;
        bool sess_found    = false;

        // $ find which session
        for (i = 0; i < NUM_USERS; i++) {
            if ((usrs_list[idx].session != NULL) && (strcmp(session_name, usrs_list[idx].session->name) == 0)) {
                sess_found = true;
                break;
            }
        }

        //% find session
        if (usrs_list[idx].session != NULL) {
            struct Session *new_session;
            new_session = malloc(sizeof(struct Session));
            strcpy(new_session->name, session_name);
            usrs_list[idx].session = new_session;

            // send JN_ACK to the user
            sprintf(data, "Session %s joined", session_name);
            sprintf(ack_msg, "%d:%d:%s:%s", JN_ACK, strlen(data), session_name, data);  // is send from another session, server is the router
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            printf("Session joined\n");
        }


        int i;
        for (i = 0; i < NUM_USERS; i++) {
            if ((user_section_list[i] != NULL) && (strcmp(session_name, user_section_list[i]->name) == 0)) {
                sess_found = true;
                break;
            }
        }

        //% update session
        if (sess_found) {
            struct Session *to_add;
            to_add = malloc(sizeof(struct Session));
            strcpy(to_add->name, session_name);
            for (i = 0; i < NUM_USERS; i++) {
                if (strcmp(users_list[i], client_id) == 0) {
                    user_section_list[i] = to_add;
                    break;
                }
            }
            //% send to the user
            sprintf(data, "Session %s joined", session_name);

            sprintf(ack_msg, "%d:%d:%s:%s", JN_ACK, strlen(data), session_name, data);  // is send from another session, server is the router
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            printf("Session joined\n");


            //% session not found
        } else {
            sprintf(data, "The session that you searched \"%s\", is not found", session_name);
            sprintf(ack_msg, "%d:%d:%s:%s", JN_NAK, strlen(data), source, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            printf("Session not found\n");
        }


        //# LEAVE_SESS
    } else if (type == LEAVE_SESS) {
        printf("Command type: LEAVE_SESS\n");
        char *client_id = msg->source;

        //% find session
        for (i = 0; i < NUM_USERS; i++) {
            if (strcmp(users_list[i], client_id) == 0) {
                if (user_section_list[i] != NULL) {  // each user can only have one session
                    user_section_list[i] = NULL;
                    break;
                }
            }
        }

        //# NEW_SESS
    } else if (type == NEW_SESS) {
        printf("Command type: NEW_SESS\n");
        bool sess_found    = false;

        char *session_name = (char *)msg->data;

        for (int i = 0; i < NUM_USERS; i++) {
            if ((usrs_list[idx].session != NULL) && (strcmp(session_name, usrs_list[idx].session->name) == 0)) {
                sess_found = true;
                break;
            }
        }
        

        int i;

        struct Session *to_add = malloc(sizeof(struct Session));
        strcpy(to_add->name, session_id);

        // maintain database
        for (i = 0; i < NUM_USERS; i++) {
            if (strcmp(users_list[i], client_id) == 0) {
                if (user_section_list[i] == NULL) {
                    user_section_list[i] = to_add;
                    sprintf(data, "Session %s created", session_id);
                    sprintf(ack_msg, "%d:%d:%s:%s", NS_ACK, strlen(data), session_id, data);
                    send(client_fd, ack_msg, strlen(ack_msg), 0);
                    break;

                    //% create a session that already exists
                } else if (strcmp(session_id, user_section_list[i]->name) == 0) {
                    // printf(data, "Session %s already exists", session_id);
                    // sprintf(data, "Session %s already exists", session_id);
                    // // * NS_NAK added
                    // sprintf(ack_msg, "%d:%d:%s:%s", NS_NAK, strlen(data), session_id, data);
                    // send(client_fd, ack_msg, strlen(ack_msg), 0);
                    break;
                }
            }
        }

        //# QUERY
    } else if (type == QUERY) {
        printf("Command type: QUERY\n");

        for (i = 0; i < NUM_USERS; i++) {
            if (users_active[i]) {  //% check if user is active
                if (user_section_list[i] == NULL) {
                    sprintf(data + strlen(data), "%s:No Session\n", users_list[i]);  // append to data
                } else {
                    sprintf(data + strlen(data), "%s:%s\n", users_list[i], user_section_list[i]->name);
                }
            }
        }

        sprintf(ack_msg, "%d:%d:%s:%s", QU_ACK, strlen(data), source, data);  // change from msg->source to source
        send(client_fd, ack_msg, strlen(ack_msg), 0);

        //# MESSAGE
    } else if (type == MESSAGE) {
        printf("Command type: MESSAGE\n");
        // loop through sockets in specified conference session sending the message
        // TODO: Need to check if in a session
        char *client_id = msg->source;

        struct Session *cur_session = NULL;
        //% find the session of the user
        for (i = 0; i < NUM_USERS; i++)
            if (users_active[i] && strcmp(users_list[i], client_id) == 0)
                cur_session = user_section_list[i];

        //% broadcast to all user in that session
        //% do it through the linked list?
        for (i = 0; i < NUM_USERS; i++)
            if (users_active[i] && (strcmp(user_section_list[i]->name, cur_session->name) == 0)) {
                sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE, strlen(msg->data), source, msg->data);  //* should be client_id?
                send(user_fds[i], ack_msg, strlen(ack_msg), 0);
                // send the message to this client
            }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: server ServerPortNumber\n");
        exit(1);
    }

    //# Set up
    char MYHOST[10];
    gethostname(MYHOST, 10);  // stay with this version, use NULL will cause error "Error in bind()"
    char *serverPortNum = argv[1];

    //$ get IP address
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
    getaddrinfo(MYHOST, serverPortNum, &hints, &res);

    //$ create socket
    int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd == -1) {
        perror("Error in socket()\n");
        return -1;
    } else
        printf("Successfully create socket()\n");

    //$ configure socket options
    int option_value = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) == -1) {
        // int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
        // {SOL_SOCKET}: specify the protocol level at the socket level
        // {SO_REUSEADDR}: allow local address reuse
        perror("Error in setsockopt(): ");
        return -1;
    } else
        printf("successfully setsockopt()");

    //$ bind socket
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Error in bind(): ");
        return -1;
    } else
        printf("Successfully bind()\n");

    //$ listen to the socket
    if (listen(server_fd, BACKLOG) == -1) {
        perror("Error in listen(): ");
        return -1;
    } else
        printf("Successfully listen()\n");
    printf("Listening for incoming messages ... \n\n");


    char rcv_buffer[MAX_GLOBAL];
    int rcv_bytes = 0;

    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);

    int new_fd = -1;
    int max_fd = server_fd;
    fd_set sock_set;  // set of user fd to select() from

    struct Message msg;  // struct Message that is sent to the users

    //# Start listening
    while (true) {
        FD_ZERO(&sock_set);  // reset
        FD_SET(server_fd, &sock_set);  // add fd to the set

        //$ update currently active users each iteration
        for (int i = 0; i < NUM_USERS; i++) {
            if (users_active[i]) {  // select the active users
                if (user_fds[i] > 0) {  //% add file descriptor to listen, s_fd = -1 for inactive users
                    FD_SET(user_fds[i], &sock_set);
                    if (user_fds[i] > max_fd)  //% update the max
                        max_fd = user_fds[i];
                }
            }
        }

        //$ select the updated socket
        if (select(max_fd + 1, &sock_set, NULL, NULL, NULL) == -1) {
            perror("Error in select(): ");
            return -1;
        }


        //$ 1. Receive new connections
        if (FD_ISSET(server_fd, &sock_set)) {
            new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
            if (new_fd < 0) {
                perror("Error in accept()");
                return -1;
            } else
                printf("New Connection established\n");

            rcv_bytes = recv(new_fd, rcv_buffer, MAX_GLOBAL, 0);
            if (rcv_bytes == -1) {
                perror("Error in rcv(): ");
                return -1;
            } else {
                printf("Message rcv()ed (New connection): \n");
                rcv_buffer[rcv_bytes] = '\0';
                printf("\t%s \n", rcv_buffer);
                parse_message(rcv_buffer, &msg);
            }
            command_handler(&msg, new_fd);
            printf("Successfully handle the command\n");


            //$ 2. Receive message from already connected users
        } else {
            for (int i = 0; i < NUM_USERS; i++) {
                if (FD_ISSET(user_fds[i], &sock_set)) {
                    rcv_bytes = recv(user_fds[i], rcv_buffer, MAX_GLOBAL, 0);

                    if (rcv_bytes == -1) {
                        perror("Error in recv(): ");
                        return -1;
                    } else {
                        printf("Message rcv()ed (From user): \n");
                        rcv_buffer[rcv_bytes] = '\0';
                        printf("\t%s \n", rcv_buffer);
                        parse_message(rcv_buffer, &msg);
                    }
                    command_handler(&msg, user_fds[i]);
                    printf("Successfully handle the command\n");
                }
            }
        }
    }


    close(server_fd);
}