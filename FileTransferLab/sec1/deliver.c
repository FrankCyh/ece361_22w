// ECE361 Lab1 deliver program
// Created by Yuhe Chen and Leting Ni
// Note. Some of the code are borowed from "talker.c" in "Beej's Guide to Network Programming"

// Description: deliver <server address> <server port number>

// Anlys: Possible Senarios of deliver
// Anlys: 1. ftp <existent_file_name>: print “A file transfer can start.” in step 3
// Anlys: 2. ftp <non_existent_file_name>: exit in step 2
// Anlys: 3. other_command <existent_file_name>: receive "no" in step 3
// Anlys: 4. other_command <non_existent_file_name>: exit in step 2


#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXBUFLEN 100

int main(int argc, char** argv) {
    // check if input number equals 2
    if (argc != 3) {
        fprintf(stderr, "usage: deliver IP_address port_number");
        exit(1);
    }

    char* serverAddr = argv[1];
    char* portNum    = argv[2];

    struct addrinfo hints, *servinfo, *p;

    //# create and set up addrinfo structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;  // dont's care IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;  // Datagram sockets

    int rv = 0;  // error type
    if (rv = getaddrinfo(serverAddr, portNum, &hints, &servinfo) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }  // infomation now store in servinfo


    int mySocketfd;

    //# create socket
    // traverse the list to find a valid one
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((mySocketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {  // create the socket
            perror("server: create socket failed");  // creation failed
            continue;
        }

        //* no need to bind, becuase it's a deliver using DATAGRAM

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);


    //# prompt user to input the message
    printf("Please input a message of the form \n \t ftp <file name> \n");

    char first[1024]  = {0};
    char second[1024] = {0};

    scanf("%s%s", first, second);


    //# file exist?
    // check the second input for existence
    bool exist = (access(second, F_OK) == 0);  // F_OK is existence test
    // return -1 for non-existent file

    if (!exist) {
        printf("File doesn't exist, exiting...\n");
        close(mySocketfd);
        exit(1);
    }

    //# send ftp
    // send message to server
    if (sendto(mySocketfd, first, strlen(first) + 1, 0, p->ai_addr, sizeof(struct sockaddr_storage)) == -1) {
        perror("deliver: sendto");
        exit(1);
    }

    //# check "yes"
    // keep accepting message from client
    char buf[1024] = {0};
    struct sockaddr_storage from;  // the server's IP address, didn't use later
    // We only use IPv4, so struct sockaddr_in should also be fine
    socklen_t fromLen = sizeof(from);

    int numBytesRecv = recvfrom(mySocketfd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromLen);
    if (numBytesRecv == -1) {
        perror("deliver: sendto");
        exit(1);
    }

    if (strcmp(buf, "yes") == 0)
        printf("A file transfer can start.\n");
    else
        printf("Expect \"yes\" \n");


    close(mySocketfd);
    return 0;
}