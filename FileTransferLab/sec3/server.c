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
#include <time.h>
#include <unistd.h>

#define MAXBUFLEN 100
#define PACKET_MESSAGE_LEN 1200

struct Packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char *filename;
    char filedata[1000];
};

int main(int argc, char **argv) {
    // check if input number equals 1
    if (argc != 2) {
        fprintf(stderr, "usage: server port_number");
        exit(1);
    }

    char *portNum = argv[1];

    //# create and set up addrinfo structure
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;  // dont's care IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;  // Datagram sockets
    hints.ai_flags    = AI_PASSIVE;  // use my IP

    int rv = 0;  // error type
    if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo)) != 0) {  // NULL indicates the local IP address
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

        break;  // found a binding
    }

    if (p == NULL) {  // fail to find a local address to bind with socket
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    // servinfo is no use right now

    // srand(time(NULL));

    //# receive message from deliver
    // infinite loop: server keeps ON all the time
    for (;;) {
        // create a buffer of length PACKET_MESSAGE_LEN to receive message
        char packet_message[PACKET_MESSAGE_LEN];
        for (int i = 0; i < PACKET_MESSAGE_LEN; i++)
            packet_message[i] = '\0';

        // create a sockaddr to get IP address and port of deliver machine
        struct sockaddr_storage from;  // used to record the host's IP address, collect in recvfrom(), used in sendto()
        // We only use IPv4, so struct sockaddr_in should also be fine
        socklen_t fromLen = sizeof(from);

        /*-------------------------------------------divider---------------------------------------------*/

        int not_done = 1;
        int i        = 0;

        FILE *fps;
        while (not_done) {
            int num_bytes_recv = recvfrom(mySocketfd, packet_message, sizeof(packet_message), 0, (struct sockaddr *)&from, &fromLen);

            if (num_bytes_recv == -1) {  // error checking
                perror("server: receive error");
                char *respondNACK = "NACK";
                sendto(mySocketfd, respondNACK, strlen(respondNACK), 0, (struct sockaddr *)&from, fromLen);
                printf("NACK\n");
                exit(1);
            }

            //# Set drop possibility to 0.01
            i++;
            double randomNumber = (double)rand() / RAND_MAX;
            // printf("%d\n", i);
            // printf("%f\n", randomNumber);
            if (randomNumber < 0.01 && i != 1) {  // randomly drop 1% of packets
                printf("Packet dropped\n");

            } else {
                //# convert packet_message to struct packet
                char temp_packet[PACKET_MESSAGE_LEN];
                strcpy(temp_packet, packet_message);
                int headerlen = 0;

                struct Packet packet;
                memset(packet.filedata, 0, 1000);

                char *total_frag_char = strtok(temp_packet, ":");  // split string into tokens

                headerlen += strlen(total_frag_char) + 1;  // +1 is for ":"
                packet.total_frag = atoi(total_frag_char);

                char *frag_no_char = strtok(NULL, ":");  // NULL: uses the position right after the end of the last token
                headerlen += strlen(frag_no_char) + 1;
                packet.frag_no = atoi(frag_no_char);

                char *size_char = strtok(NULL, ":");
                headerlen += strlen(size_char) + 1;
                packet.size = atoi(size_char);

                char *filename_char = strtok(NULL, ":");
                headerlen += strlen(filename_char) + 1;
                packet.filename = filename_char;

                // printf("total_frag:%d, frag_no:%d, size:%d, filename:%s, headerlen: %d\n",
                // packet.total_frag, packet.frag_no, packet.size, packet.filename, headerlen);

                // copy char one-by-one from packet.filedata to packet_message
                int index = headerlen;
                for (int i = 0; i < packet.size; i++) {
                    char c             = packet_message[index];
                    packet.filedata[i] = c;
                    index++;
                }
                // printf("Length is %d; filedata is %s\n", packet.size, packet.filedata);

                //# write packet data to file
                if (packet.frag_no == 1)  // open a file in write mode for the first fragment
                    fps = fopen(packet.filename, "w");

                fwrite(packet.filedata, 1, packet.size, fps);

                if (packet.frag_no == packet.total_frag) {
                    // break out of the loop for the last fragment
                    fclose(fps);
                    not_done = 0;  // terminate while loop
                }

                //# send ACK to deliver if previous steps are done
                char *respondACK = "ACK";
                if (sendto(mySocketfd, respondACK, strlen(respondACK), 0, (struct sockaddr *)&from, fromLen) == -1) {
                    perror("sendto failed");
                    exit(1);
                }
                /// printf("Replied ACK, frag no = %d\n", packet.frag_no);
            }
        }
        printf("File transfer successful!\n");
    }

    close(mySocketfd);
    return 0;
}