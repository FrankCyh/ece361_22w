#include <errno.h>
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

//# Database
#define NUM_USERS 5
#define MAX_NUM_SESSION 20  // maximum number of session in database sess_list

//$ Users
struct Sessions {  // keep sessions of a user
    char *name;
    struct Sessions *next;
};

struct User {
    char *name;
    char *password;
    int usr_fd;
    bool active;
    struct Sessions *sessions;
};

// initialize database
struct User usrs_list[NUM_USERS] = {
    {"Frank", "frank", -1, false, NULL},
    {"Nick", "nick", -1, false, NULL},
    {"Jack", "jack", -1, false, NULL},
    {"William", "william", -1, false, NULL},
    {"Hamid", "hamid", -1, false, NULL},
};

// helper function to get index from user name
int getUsrIdx(unsigned char *usr_name) {
    for (int i = 0; i < NUM_USERS; i++)
        if (strcmp(usrs_list[i].name, usr_name) == 0)
            return i;
    printf("Wrong user, no record in the database \n");
    return -1;
}

//$ Session
struct UsrsInSession {
    int usr_id;
    struct UsrsInSession *next;
};

struct SessionUsers {  // a list of session and the user in the session; update using update_sess_list
    char *session_id;
    struct UsrsInSession *usr_in_sess;
};

struct SessionUsers *sess_list[MAX_NUM_SESSION] = {NULL};
int num_sess                                    = 0;  // record the total number of sessions right now

// helper function for extracting session information from usrs_list
void update_sess_list() {
    memset(sess_list, 0, MAX_NUM_SESSION * sizeof(struct SessionUsers *));
    num_sess = 0;

    char *new_session_id;
    struct Sessions *itr       = NULL;
    struct UsrsInSession *itr2 = NULL;

    // Found all the distinct session_ids
    for (int i = 0; i < NUM_USERS; i++) {
        for (itr = usrs_list[i].sessions; itr != NULL; itr = itr->next) {
            bool new       = true;
            new_session_id = itr->name;
            for (int j = 0; j < num_sess; j++) {
                if (strcmp(new_session_id, sess_list[j]->session_id) == 0)
                    new = false;
            }

            if (new) {
                struct SessionUsers *new_sess = malloc(sizeof(struct SessionUsers));
                char *tmpchar                 = malloc(20);
                strcpy(tmpchar, new_session_id);
                new_sess->session_id  = tmpchar;
                new_sess->usr_in_sess = NULL;
                sess_list[num_sess]   = new_sess;
                num_sess++;
            }
        }
    }

    for (int j = 0; j < num_sess; j++) {
        for (int i = 0; i < NUM_USERS; i++) {
            for (itr = usrs_list[i].sessions; itr != NULL; itr = itr->next) {
                if (strcmp(sess_list[j]->session_id, itr->name) == 0) {
                    struct UsrsInSession *new_usr_in_sess = malloc(sizeof(struct UsrsInSession));
                    new_usr_in_sess->usr_id               = i;
                    new_usr_in_sess->next                 = NULL;
                    if (sess_list[j]->usr_in_sess == NULL)
                        sess_list[j]->usr_in_sess = new_usr_in_sess;
                    else {
                        for (itr2 = sess_list[j]->usr_in_sess; itr2->next != NULL; itr2 = itr2->next)
                            ;  // go to the end of linked list
                        itr2->next = new_usr_in_sess;
                    }
                }
            }
        }
    }
}

//# Helper function
// handles client request based on parsed message received
void command_handler(struct Message *msg, int client_fd) {
    //$ information from struct Message* msg
    int type = msg->type;  // type
    char client_id[MAX_NAME];  // source
    strcpy(client_id, msg->source);  // if use char* and does not malloc enough space will cause segmentation fault
    char *session_id;  // session_id sometimes passed in as data

    int idx = getUsrIdx(client_id);

    //$ ack_msg sent back to client
    char ack_msg[MAX_MESSAGE];
    memset(ack_msg, 0, sizeof(ack_msg));
    char *source = "Server";  // source of sent ack_msg
    char *data   = malloc(MAX_DATA);  // data field in the ack_msg
    char *buffer = malloc(MAX_DATA);  // buffer used to process string

    //$ session create and join
    struct Sessions *to_add;
    to_add                     = malloc(sizeof(struct Sessions));
    to_add->name               = malloc(MAX_NAME);
    to_add->next               = NULL;
    struct Sessions *itr       = NULL;
    struct UsrsInSession *itr2 = NULL;

    int i;  // used for loop and index

    //# LOGIN
    if (type == LOGIN) {
        // Anlys: result based on two conditions: 1. whether user is already logged in; 2. whether user's password is correct
        // Corner: If the user haven't log out manually, it would be still logged in on the server's databse, thus the same user will not be able to log in
        if (strcmp(usrs_list[idx].password, (char *)msg->data) == 0) {
            if (!usrs_list[idx].active) {
                // ACK: Not logged in yet
                usrs_list[idx].active = true;
                usrs_list[idx].usr_fd = client_fd;

                data = "";  // Data field should be empty
                sprintf(ack_msg, "%d:%d:%s:%s", LO_ACK, strlen(data), source, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
                print_msg(ack_msg, true);
                printf("User %s successfully log in\n", usrs_list[idx].name);

            } else {
                // NACK: Already logged in
                data = "Already logged in";
                sprintf(ack_msg, "%d:%d:%s:%s", LO_NAK, strlen(data), msg->source, data);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
                print_msg(ack_msg, true);
                printf("Log in failed: %s\n", data);
            }

        } else {
            // NACK: Incorrect password
            data = "Incorrect password for user";
            sprintf(ack_msg, "%d:%d:%s:%s", LO_NAK, strlen(data), source, data);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
            close(client_fd);
            print_msg(ack_msg, true);
            printf("Log in failed: %s\n", data);
        }



        //# LOGOUT
    } else if (type == LOGOUT) {  // logout or quit request
        usrs_list[idx].active   = false;
        usrs_list[idx].sessions = NULL;
        usrs_list[idx].usr_fd   = -1;
        close(client_fd);  // close the client socket
        printf("User %s successfully logged out.\n", usrs_list[idx].name);



        //# NEW_SESS
    } else if (type == NEW_SESS) {  // create session results
        // Corner: duplicate session of already created Sessions
        // There is no limit of the number of session a user can create or join

        session_id = (char *)msg->data;

        // create new session's name
        strcpy(to_add->name, session_id);

        //$ Check duplicate session of all user sessions
        bool duplicate_sess = false;
        for (i = 0; i < NUM_USERS && !duplicate_sess; i++) {
            if (usrs_list[i].sessions != NULL) {
                itr = usrs_list[i].sessions;
                do {
                    if (strcmp(itr->name, session_id) == 0) {
                        duplicate_sess = true;
                        break;
                    }
                    itr = itr->next;
                } while (itr != NULL);
            }
        }

        //$ Found duplicate sessions
        if (duplicate_sess) {
            strcpy(data, "Session ");
            strcat(data, (char *)session_id);
            strcat(data, " already exists");

            sprintf(ack_msg, "%d:%d:%s:%s", NS_NAK, strlen(data), session_id, data);
            print_msg(ack_msg, true);
            send(client_fd, ack_msg, strlen(ack_msg), 0);

            //$ Create new session and add to end of session list of usrs_list[idx]
        } else {
            // add the session at the end of the linked list
            if (usrs_list[idx].sessions == NULL)  // no session yet
                usrs_list[idx].sessions = to_add;
            else {
                for (itr = usrs_list[idx].sessions; itr->next != NULL; itr = itr->next)
                    ;  // go to the end of linked list
                itr->next = to_add;
            }

            strcpy(data, "Session ");
            strcat(data, (char *)session_id);
            strcat(data, " created");

            sprintf(ack_msg, "%d:%d:%s:%s", NS_ACK, strlen(data), session_id, data);
            print_msg(ack_msg, true);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
        }



        //# JOIN
    } else if (type == JOIN) {  // join session request
        // Corner: session to JOIN is not found
        char *session_id = (char *)msg->data;

        // create new session's name
        strcpy(to_add->name, session_id);

        bool sess_found = false;
        bool already_in = false;
        struct Sessions *sess_to_join;

        for (i = 0; i < NUM_USERS && !sess_found; i++) {
            if (usrs_list[i].sessions != NULL) {
                itr = usrs_list[i].sessions;
                do {
                    if (strcmp(itr->name, session_id) == 0) {
                        sess_found   = true;
                        sess_to_join = itr;
                        break;
                    }
                    itr = itr->next;
                } while (itr != NULL);
            }
        }

        //$ Session to join is not found
        if (!sess_found) {
            strcpy(data, "Session ");
            strcat(data, (char *)session_id);
            strcat(data, " not found");
            sprintf(ack_msg, "%d:%d:%s:%s", JN_NAK, strlen(data), session_id, data);
            print_msg(ack_msg, true);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
        } else {
            //$ Check if this user have already join the session
            for (itr = usrs_list[idx].sessions; itr != NULL; itr = itr->next)
                if (strcmp(itr->name, sess_to_join->name) == 0) {
                    already_in = true;
                    break;
                }


            //$ Already joined: NAK
            if (already_in) {
                strcpy(data, "User is already in Session ");
                strcat(data, (char *)session_id);
                sprintf(ack_msg, "%d:%d:%s:%s", JN_NAK, strlen(data), session_id, data);
                print_msg(ack_msg, true);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
            }


            //$ Create new session and add to end of session list of usrs_list[idx]
            else {
                // add the session at the end of the linked list
                if (usrs_list[idx].sessions == NULL)  // no session yet
                    usrs_list[idx].sessions = to_add;
                else {
                    for (itr = usrs_list[idx].sessions; itr->next != NULL; itr = itr->next)
                        ;  // go to the end of linked list
                    itr->next = to_add;
                }

                strcpy(data, "Session ");
                strcat(data, (char *)session_id);
                strcat(data, " joined");
                sprintf(ack_msg, "%d:%d:%s:%s", JN_ACK, strlen(data), session_id, data);
                print_msg(ack_msg, true);
                send(client_fd, ack_msg, strlen(ack_msg), 0);
            }
        }



        //# LEAVE_SESS
    } else if (type == LEAVE_SESS) {
        // Corner: user is not in the session
        char *session_id      = (char *)msg->data;
        bool in_sess          = true;
        struct Sessions *prev = NULL;

        if (usrs_list[idx].sessions == NULL)  // user didn't join any sessions
            in_sess = false;
        else {
            for (itr = usrs_list[idx].sessions; itr != NULL; prev = itr, itr = itr->next)
                if (strcmp(itr->name, session_id) == 0) {
                    if (prev == NULL)  // delete the first node
                        usrs_list[idx].sessions = itr->next;  //* can't use itr, because it points to somewhere else
                    else
                        prev->next = itr->next;  // delete the node itr from the list
                    break;  // found the session to leave
                }

            // traversed through the list and not find
            if ((itr == NULL && prev != NULL))
                in_sess = false;  // note must exclude the case the first node is found and then deleted, which is the case itr == NULL && prev == NULL
        }


        if (!in_sess) {
            strcpy(data, "User is not in Session ");
            strcat(data, (char *)session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", LS_NAK, strlen(data), session_id, data);
            print_msg(ack_msg, true);
            send(client_fd, ack_msg, strlen(ack_msg), 0);

        } else {
            strcpy(data, "User successfully leaves Session ");
            strcat(data, (char *)session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", LS_ACK, strlen(data), session_id, data);
            print_msg(ack_msg, true);
            send(client_fd, ack_msg, strlen(ack_msg), 0);
        }



        //# QUERY
    } else if (type == QUERY) {
        // no corner case, always acknowledge
        update_sess_list();
        for (i = 0; i < num_sess; i++) {
            sprintf(data + strlen(data), "NO.%d: %s \n", i + 1, sess_list[i]->session_id);
            for (itr2 = sess_list[i]->usr_in_sess; itr2 != NULL; itr2 = itr2->next)
                sprintf(data + strlen(data), "\t %s \n", usrs_list[itr2->usr_id].name);
        }
        sprintf(ack_msg, "%d:%d:%s:%s", QU_ACK, strlen(data), source, data);  // change from msg->source to source
        print_msg(ack_msg, true);
        send(client_fd, ack_msg, strlen(ack_msg), 0);


        //# MESSAGE
    } else if (type == MESSAGE) {
        //* message is of the format <\session_to_send> <messages_content>
        // Corner: the sesssion doesn't exist
        // Corner: user is not in the sesssion
        update_sess_list();

        int from_usr_fd = usrs_list[idx].usr_fd;
        strcpy(buffer, msg->data);
        session_id = strtok(buffer, " ");  // the first one is the session id
        buffer     = buffer + strlen(session_id) + 1;  // the rest is the message to broadcast, 1 is for the space

        // Check
        bool in_sess = false;
        for (itr = usrs_list[idx].sessions; itr != NULL; itr = itr->next)
            if (strcmp(itr->name, session_id) == 0) {
                in_sess = true;
                break;
            }

        //$ Check if the session exist
        int sess_idx = -1;  // index of the found session
        // check only whether user is in session is enough, but this existance check provide more detailed explanation
        for (i = 0; i < num_sess; i++) {
            if (strcmp(session_id, sess_list[i]->session_id) == 0) {
                sess_idx = i;
                break;
            }
        }

        if (sess_idx == -1 || !in_sess) {
            if (sess_idx == -1)
                sprintf(data, "There is no session %s", session_id);
            else if (!in_sess)
                sprintf(data, "User %s is not in session %s", usrs_list[idx].name, session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE_NAK, strlen(data), source, data);
            print_msg(ack_msg, true);
            send(from_usr_fd, ack_msg, strlen(ack_msg), 0);
        } else {
            // Send ack to from_client
            sprintf(data, "Successfully broadcast to session %s", session_id);
            sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE_ACK, strlen(data), source, data);
            print_msg(ack_msg, true);
            send(from_usr_fd, ack_msg, strlen(ack_msg), 0);

            // Broadcast to all other users in the session
            for (itr2 = sess_list[sess_idx]->usr_in_sess; itr2 != NULL; itr2 = itr2->next) {
                if (usrs_list[itr2->usr_id].usr_fd != from_usr_fd) {
                    sprintf(ack_msg, "%d:%d:%s:%s", MESSAGE, strlen(data), session_id, buffer);  // source is the conference session_id
                    print_msg(ack_msg, true);
                    send(usrs_list[itr2->usr_id].usr_fd, ack_msg, strlen(ack_msg), 0);
                }
            }
        }



        //# PRIVATE
    } else if (type == PRIVATE) {
        // Corner: the user to send to doesn't exit in the database or isn't active
        // Corner: send message to himself
        int from_usr_fd = usrs_list[idx].usr_fd;
        strcpy(buffer, msg->data);
        char *usr_to_send   = strtok(buffer, " ");  // the first one is the user name to send to
        int usr_to_send_idx = -1;
        buffer              = buffer + strlen(usr_to_send) + 1;  // the rest is the private message

        //$ Check if the user is active
        bool usr_found = false;
        for (int i = 0; i < NUM_USERS; i++)
            if (usrs_list[i].active && strcmp(usrs_list[i].name, usr_to_send) == 0) {
                usr_found       = true;
                usr_to_send_idx = i;
                break;
            }

        if (!usr_found) {
            // user with name usr_to_send is not found, send NAK
            sprintf(data, "User %s is not found", usr_to_send);
            sprintf(ack_msg, "%d:%d:%s:%s", PRIVATE_NAK, strlen(data), source, data);
            print_msg(ack_msg, true);
            send(from_usr_fd, ack_msg, strlen(ack_msg), 0);
        } else if (idx == usr_to_send_idx) {
            sprintf(data, "Can't send a private message to yourself ");
            sprintf(ack_msg, "%d:%d:%s:%s", PRIVATE_NAK, strlen(data), source, data);
            print_msg(ack_msg, true);
            send(from_usr_fd, ack_msg, strlen(ack_msg), 0);
        } else {
            // Send ack to from_user
            sprintf(data, "Successfully send private message to %s", usr_to_send);
            sprintf(ack_msg, "%d:%d:%s:%s", PRIVATE_ACK, strlen(data), source, data);
            print_msg(ack_msg, true);
            send(from_usr_fd, ack_msg, strlen(ack_msg), 0);

            // Send privately to user with name usr_to_send
            sprintf(ack_msg, "%d:%d:%s:%s", PRIVATE, strlen(data), usrs_list[idx].name, buffer);  // source is the conference session_id
            print_msg(ack_msg, true);
            send(usrs_list[usr_to_send_idx].usr_fd, ack_msg, strlen(ack_msg), 0);
        }
    }

    printf("\n");
}

#define BACKLOG 10  // used for listen



int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: server ServerPortNumber\n");
        exit(1);
    }
    char *serverPortNum = argv[1];

    //# Set up
    char MYHOST[10];
    gethostname(MYHOST, 10);

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
        perror("Server: socket");
        exit(1);
    }

    //$ configure socket options
    int option_value = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) == -1) {
        // int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
        // {SOL_SOCKET}: specify the protocol level at the socket level
        // {SO_REUSEADDR}: allow local address reuse
        perror("Server: setsockopt");
        exit(1);
    }

    //$ bind socket
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Server: bind");
        exit(1);
    }

    //$ listen to the socket
    if (listen(server_fd, BACKLOG) == -1) {
        perror("Server: listen");
        exit(1);
    }

    printf("Listening for incoming messages...\n\n");

    //$ Preparation for main loop
    char rcv_buffer[MAX_MESSAGE];

    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);

    int new_fd = -1;
    int max_fd = server_fd;
    fd_set sock_set;  // set of user fd to select() from

    int num_bytes_rcvd = 0;
    struct Message sent_msg;  // struct Message that is sent to the users

    //# Main loop
    while (true) {
        FD_ZERO(&sock_set);  // reset
        FD_SET(server_fd, &sock_set);  // add fd to the set

        // add the active client fds to the set
        for (int i = 0; i < NUM_USERS; i++) {
            if (usrs_list[i].active) {
                int s_fd = usrs_list[i].usr_fd;
                if (s_fd > 0) {
                    FD_SET(s_fd, &sock_set);
                    max_fd = s_fd > max_fd ? s_fd : max_fd;  // update the max file descriptor
                }
            }
        }

        //$ select the updated socket
        if (select(max_fd + 1, &sock_set, NULL, NULL, NULL) == -1) {
            perror("Server: select");
            exit(1);
        }


        //$ 1. Receive new connections
        if (FD_ISSET(server_fd, &sock_set)) {
            new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
            if (new_fd < 0) {
                perror("Server: accept");
                exit(1);
            }

            num_bytes_rcvd = recv(new_fd, rcv_buffer, MAX_MESSAGE, 0);

            if (num_bytes_rcvd < 0) {
                perror("Server: recv");
                exit(1);
            }
            rcv_buffer[num_bytes_rcvd] = '\0';

            print_msg(rcv_buffer, false);
            parse_message(rcv_buffer, &sent_msg);
            command_handler(&sent_msg, new_fd);

            //$ 2. Receive message from already connected users
        } else {
            for (int i = 0; i < NUM_USERS; i++) {
                if (FD_ISSET(usrs_list[i].usr_fd, &sock_set)) {
                    num_bytes_rcvd = recv(usrs_list[i].usr_fd, rcv_buffer, MAX_MESSAGE, 0);

                    if (num_bytes_rcvd < 0) {
                        perror("Server: recv");
                        exit(1);
                    }
                    rcv_buffer[num_bytes_rcvd] = '\0';

                    print_msg(rcv_buffer, false);
                    parse_message(rcv_buffer, &sent_msg);
                    command_handler(&sent_msg, usrs_list[i].usr_fd);
                }
            }
        }
    }

    close(server_fd);
}
