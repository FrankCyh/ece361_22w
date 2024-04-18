// ECE361 Lab1 server program
// Created by Yuhe Chen and Leting Ni
// Note. Some of the code are borowed from "listener.c" in "Beej's Guide to Network Programming"

// Description: Input Format: server <UDP listen port>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXBUFLEN 100

int main(int argc, char** argv) {
    // check if input number equals 1
    if (argc != 2) {
        fprintf(stderr, "usage: server port_number");
        exit(1);
    }

    char* portNum = argv[1];

    //# create and set up addrinfo structure
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;  // dont's care IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;  // Datagram sockets
    hints.ai_flags    = AI_PASSIVE;  // use my IP

    int rv = 0;  // error type
    if (rv = getaddrinfo(NULL, portNum, &hints, &servinfo) != 0) {  // NULL indicates the local IP address
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }  // infomation now store in servinfo

    //# create socket and combine with port on local machine
    int mySocketfd;  // my socket file descriptor

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((mySocketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {  // create the socket with domain PF_INET (IPv4), type SOCK_DGRAM
            perror("server: create socket failed");  // creation failed
            continue;
        }

        // bind the socket with the port on local machine
        if (bind(mySocketfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(mySocketfd);
            perror("server: bind socket and IP failed");
            continue;
        }

        break;
    }

    if (p == NULL) { // fail to find a local address to bind with socket
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    // servinfo is no use right now

    //# receive message from deliver
    // create a buffer of length MAXBUFLEN to receive message
    char buf[MAXBUFLEN] = {0};

    // create a sockaddr to get IP address and port of deliver machine
    struct sockaddr_storage from;  // used to record the host's IP address, collect in recvfrom(), used in sendto()
    // We only use IPv4, so struct sockaddr_in should also be fine
    socklen_t fromLen = sizeof(from);

    while (1) {
        int numBytesRecv = recvfrom(mySocketfd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromLen);
        if (numBytesRecv == -1) {
            perror("server: receive error");
            exit(1);
        }
        printf("Message received: \"%s\"\n", buf);

        //# feedback to deliver
        char* respondMsg = {0};
        if (strcmp(buf, "ftp") == 0)
            respondMsg = "yes";
        else
            respondMsg = "no";  // file exists in the folder but the command is not ftp

        if (sendto(mySocketfd, respondMsg, strlen(respondMsg) + 1, 0, (struct sockaddr*)&from, fromLen) == -1) {
            perror("sendto failed");
            exit(1);
        }

        printf("Message replied: %s\n", respondMsg);
    }

    close(mySocketfd);
    return 0;
}