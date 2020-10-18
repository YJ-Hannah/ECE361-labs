#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include "packet.h"

#define MAXDATASIZE 4096 //got this number from my ECE344 lab, subject to 
#define MAXBUFLEN 4096
#define BITE_LIMIT 1000
#define ACK "ACK"
#define NACK "NACK"

// the execution command should have the following structure: deliver <server address> <server port number>
////////////////////////////////////////////////////////////////////////////////////
//// upon execution the client should:                                          ////
////  1. Ask the user to input a message as follows: ftp <file name>            ////
////  2. Check the existence of the file                                        ////
////      a. if the message is "yes", print out "A file transfer can start"     ////
////      b. else, exit                                                         ////
////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv){
    //===================Section 1=====================================================
    // (see page 35 on Beej's guide for useful tips)

    int sentftp;
    int sockfd;
    char buf[MAXBUFLEN];
    struct addrinfo *servinfo, hints;
    socklen_t addrlen;
    struct sockaddr_storage server_sock; // connector address info
    int dummy;
    char *ftp = "ftp";
    char * port = argv[2];

    clock_t start, end;
    double cpu_time_used;

    if (argc != 3) { //input format: deliver <server address> <server port number>
        printf("usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // find the IP at the specified port (FIND THE SERVER)
    if ((dummy = getaddrinfo(argv[1], port, &hints, &servinfo)) != 0) {
        perror("bad retrieval");
        return 1;
    }

    // get socket from server port
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror("server: socket");
    }

    printf("please enter filename to transfer: ftp <file name>\n");
    char key[10], file[35]; //HERE FILE CONTAINS THE .txt or .img
    scanf("%s %s", key, file);

    if(strcmp(key, "ftp")){
        perror("invalid input: exiting");
        exit(1);
    }

    if(access(file, F_OK)!=0){
        printf("file DNE\n");
        exit(1);
    } else{
        printf("file found\n");
    }
    

    // stuff Beej's guide recommends
    printf("client: connecting... \n");
     
    start = clock(); // start measuring RTT
    if((sentftp = sendto(sockfd, (const char *)ftp, strlen(ftp), 0, servinfo->ai_addr, servinfo->ai_addrlen)) == -1){
        perror("failed to send ftp");
        exit(1);
    } else{
        printf("message sent successfully\n");
        // sent to the server. Now we wait for a response back to complete the round trip
    }
    
    
    // receive a response from the server
    addrlen = sizeof(struct sockaddr_storage);
    int nbits =  recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&server_sock, &addrlen);
    
    //problems may occur here
    buf[nbits] = '\0';

    //printf("value of buffer: %s\n", buf);
    if(nbits == -1){
        perror("failed to receive from server");
        exit(1);
    }
    printf("nbits: %d\n", nbits);
    if(strcmp(buf, "yes") == 0){
        printf("A file transfer can start\n");
        end = clock();
    } else {
        printf("file transfer may not proceed\n");
        exit(1);
    }
 
    //===================Section 2&3=====================================================
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///// In this section, you will implement a client and a server to transfer a file.                       /////
    ///// Unlike simply receiving a message and sending it back, you are required to have                     /////
    ///// a specific packet format and implement acknowledge for the simple file transfer using UDP socket.   /////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Section 2: How long is the measured round trip time? (about 7 microseconds for lab1)
    int RTT = ((double) (end - start)) / CLOCKS_PER_SEC *1000000; // calculate RTT in micro seconds
    printf("RTT = %d microseconds \n", RTT);

    // Section 3: File transfer
    // if the file is larger than 1000 bites, fragment it before transmission
  
    //step 1. fragment the file, if needed.
    //step 2. implement the packet format for each fragment.
    //step 3. iteratively send each packet (1 fragment) until none are left
    //step 4. implement stop-and-wait i.e. don't proceed to until an ACK

    //numFrag stores the total number of fragmentation
    int numFrag;
    int sendbits;
    char **packets = fragment_this(file, &numFrag); //get the fragmented packets in string
    memset(buf, 0, sizeof(char) * MAXBUFLEN);
    //send each packet until none is left
    for (int packNo = 0; packNo < numFrag; packNo++) {
        printf("Sending packet %d (total: %d)\n", packNo + 1, numFrag);
        if (sendbits = sendto(sockfd, packets[packNo], MAXBUFLEN, 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            printf("failed to send packet #%d\n", packNo + 1); 
            exit(1);
        }  
   	
	printf("Sent packet %d (total: %d)\n", packNo + 1, numFrag);
	memset(buf, 0, sizeof(char) * MAXBUFLEN);
	if (nbits = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&server_sock, &addrlen) == -1) {
	    printf("failed to recieve ackownledgement for packet #%d\n", packNo + 1);
	    exit(1);
	}
	
	if (strcmp(buf, ACK) == 0) {
	    printf("packet #%d recieved\n", packNo + 1);
	}
	else { //if NACK is recieved keep the packNo unchanged keep sending the same packet
	    printf("packet #%d not recieved\n", packNo + 1);
	    packNo--;  
	}
    }
    //free the char array for the packets 
    free_fragments(packets, numFrag);


    close(sockfd);
    return 0;
    
}
