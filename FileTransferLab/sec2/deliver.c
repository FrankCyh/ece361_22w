// ECE361 Lab2 Section 1 measured round-trip time
// Created by Yuhe Chen and Leting Ni
// Note. Some of the code are borowed from "talker.c" in "Beej's Guide to Network Programming"

// Description: deliver <server address> <server port number>

// Anlys: Possible Scenarios of deliver
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
#include <time.h>
#include <unistd.h>

#define MAXBUFLEN 100
#define PACKET_MESSAGE_LEN 1200

struct Packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

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
    if ((rv = getaddrinfo(serverAddr, portNum, &hints, &servinfo)) != 0) {
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

        //* no need to bind, because it's a deliver using unconnected DATAGRAM, only one-way communication

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }


    //# prompt user to input the message
    printf("Please input a message of the form \n \t ftp <file name> \n");

    char first[1024]  = {0};
    char second[1024] = {0};
    scanf("%s%s", first, second);

    // char first[1024]  = "ftp";
    // char second[1024] = "a.txt";

    //# file exist?
    // check the second input for existence
    bool exist = (access(second, F_OK) == 0);  // F_OK is existence test
    // return -1 for non-existent file

    if (!exist) {
        printf("File doesn't exist, exiting...\n");
        close(mySocketfd);
        exit(1);
    }

    struct sockaddr_storage from;  // the server's IP address, didn't use later
    // We only use IPv4, so struct sockaddr_in should also be fine
    socklen_t fromLen = sizeof(from);


    /*-------------------------------------------divider---------------------------------------------*/

    //# Find file length
    FILE* fp;
    fp = fopen(second, "r");  // open file with read mode
    fseek(fp, 0, SEEK_END);  // Sets the position indicator; SEEK_END: End of file
    long file_len = ftell(fp);  // Position indicator of the stream is at the end, thus return the length of the file

    int total_frag_num = file_len / 1000 + 1;
    rewind(fp);  // back to top

    // printf("\nFile was opened. File length is %ld. %d packets in total.\n", file_len, total_frag_num);


    int is_ack = 1;  // check if received ACK from server
    char packet_message[PACKET_MESSAGE_LEN];
    char temp[20];  // used for transfering total_frag/frag_no/size to string; 20 is a proper length

    for (int fragNo = 1; fragNo <= total_frag_num && is_ack; fragNo++) {
        is_ack = 0;  // change back to 1 if receive ACK

        //# initialize struct packet
        struct Packet packet;
        memset(packet.filedata, 0, 1000);  // clear the filedata(ptr, 1000 char) to 0
        packet.total_frag = total_frag_num;
        packet.frag_no    = fragNo;

        if (fragNo == total_frag_num) {  // last packet, size less than 1000
            packet.size = file_len % 1000;
        } else {
            packet.size = 1000;  // not last
        }

        packet.filename = second;
        // printf("frag_no:%d, size:%d, filename:%s\n", packet.frag_no, packet.size, packet.filename);

        fread(packet.filedata, sizeof(char), packet.size, fp);  // read from fp to packet

        //# configure packet_message
        memset(packet_message, 0, PACKET_MESSAGE_LEN);  // initialize packet_message to all '\0'
        memset(temp, 0, sizeof(temp));  // initialize temp, all '\0'

        sprintf(temp, "%d", packet.total_frag);
        strcat(packet_message, temp);
        strcat(packet_message, ":");

        sprintf(temp, "%d", packet.frag_no);
        strcat(packet_message, temp);
        strcat(packet_message, ":");

        sprintf(temp, "%d", packet.size);
        strcat(packet_message, temp);
        strcat(packet_message, ":");

        strcat(packet_message, packet.filename);
        strcat(packet_message, ":");

        // copy char one-by-one from packet.filedata to packet_message, can't use strcat because it's binary file
        int index = strlen(packet_message);
        for (int i = 0; i < packet.size; i++) {
            char c                = packet.filedata[i];
            packet_message[index] = c;
            index++;
        }
        // now index is the total length of packet message
        // if (fragNo == total_frag_num) printf("Length is %d; Packet is %s\n", index, packet_message);

        //# send and receive data
        int num_bytes_sent = 0, num_bytes_recv = 0;
        char buffer[1000];

        num_bytes_sent = sendto(mySocketfd, packet_message, index, 0, p->ai_addr, sizeof(struct sockaddr_storage));
        if (num_bytes_sent == -1) {
            perror("deliver: sendto");
            exit(1);
        }

        num_bytes_recv = recvfrom(mySocketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromLen);
        if (num_bytes_recv == -1) {
            perror("deliver: recvfrom");
            exit(1);
        }

        buffer[num_bytes_recv] = '\0';  // conver to a string, used for strcmp with "ACK"


        //# check ACK from server
        if (strcmp(buffer, "ACK") == 0) {
            printf("Received \"ACK\" from server.\n");
            is_ack = 1;
        } else {
            printf("Did not receive \"ACK\" from server.\n");
            is_ack = 0;
        }
    }

    freeaddrinfo(servinfo);
    close(mySocketfd);
    fclose(fp);
    return 0;
}