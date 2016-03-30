/* Name: ZhaoxuanXu
   Student Number: V00791274

*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

//RDP packet
#define MAXBUFLEN 65536
#define WINDOW_SIZE 10240 
#define DAT 0
#define ACK 1
#define SYN 2
#define FIN 3
#define RST 4

struct packetheader {
    int type;
    int number;
    int info;
    char *data;
	char wastepack;
	char numbersofchars;
} packetheader;

//Globals for connection
int sockfd;
struct sockaddr_in sender_addr;
socklen_t sender_len;
int receiver_port;
char *sender_file_name;
struct timeval begin, end;
struct sockaddr_in receiver_addr;
socklen_t receiver_len;
char *receiver_ip;
int window;
int seq_no;
int syn = 0;
int questionmark;
int z=0;


int rrst = 0;
int ack = 0;
int rst = 0;
int dat = 0;

int fin = 0;





//print packet info
void packet_message(char event, int type, unsigned int number, unsigned int info) {
    char *s_addr;
    char r_addr[16];
    struct timeval tv;
    unsigned int h, mcm, sos, us;

    //get current time
    gettimeofday(&tv, NULL);
    h = (tv.tv_sec/3600-8) % 24;
    mcm = tv.tv_sec / 60 % 60;
    sos = tv.tv_sec % 60;
    us = tv.tv_usec;

    //get sender and receiver IP addresses
    s_addr = inet_ntoa(sender_addr.sin_addr);
    strncpy(r_addr, receiver_ip, 16);

    //print packet info
    switch(type) {
        case ACK:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d ACK %u %u\n", h, mcm, sos, us,
                event, r_addr, receiver_port, s_addr, receiver_port,
                number, info);
            break;
        case DAT:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d DAT %u %u\n", h, mcm, sos, us,
                event, r_addr, receiver_port, s_addr, receiver_port,
                number, info);
            break;
        case FIN:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d FIN %u 0\n", h, mcm, sos, us,
                event, r_addr, receiver_port, s_addr, receiver_port,
                number);
            break;
        case SYN:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d SYN %u 0\n", h, mcm, sos, us,
                event, r_addr, receiver_port, s_addr, receiver_port,
                number);
            break;
        case RST:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d RST\n", h, mcm, sos, us,
                event, r_addr, receiver_port, s_addr, receiver_port);
            break;
        default:
		printf("IMHERERERERE\n");
            fprintf(stderr, "invalid packet\n");    
    }
}

//handle the packet from sender
int rdp_receive(char *data, size_t *read) {

    char buffer[1024];
    char *token, *fcontent;
    char event_s, rcase;
    struct packetheader packet;
    int pack, numbytes;
    *read = 0;

    window = WINDOW_SIZE;

    while(MAXBUFLEN - *read > 959) {
    
        bzero(buffer, strlen(buffer));
        numbytes = recvfrom(sockfd, buffer, 1024, 0, NULL, NULL);

        

        //read from packet
        fcontent = strstr(buffer, "\n\n");
        *fcontent = '\0';
        packet.data = fcontent+2;
        token = strtok(buffer, " :\t\n");
        token = strtok(NULL, " :\t\n");
        if(strcmp(token, "CSc361") != 0) {
		printf("IMHERERERERE\n");
            perror("invalid packet");
            return -1;
        }
		for(z=0; z<2; z++){
			token = strtok(NULL, " :\t\n");
		}

        //determine the type of the packet
        if(strcmp(token, "DAT") == 0) {
            packet.type = DAT;
			for(z=0; z<2; z++){
				token = strtok(NULL, " :\t\n");
			}
            packet.number = atoi(token);
			for(z=0; z<2; z++){
				token = strtok(NULL, " :\t\n");
			}
            packet.info = atoi(token);
        }
        else if(strcmp(token, "RST") == 0) {
            packet.type = RST;
        }
        else if(strcmp(token, "SYN") == 0){ 
            packet.type = SYN;
			for(z=0; z<2; z++){
				token = strtok(NULL, " :\t\n");
			}
            packet.number = atoi(token);
        }
	else if(strcmp(token, "FIN") == 0) {
	    packet.type = FIN;
			for(z=0; z<2; z++){
				token = strtok(NULL, " :\t\n");
			}
            packet.number = atoi(token);
	    }
	    else {
            perror("invalid packet");
            return -1;
        }

        bzero(buffer, strlen(buffer));

        //check if the received packet is duplicated
        if(packet.number < seq_no) {
            rcase = 'R';
            event_s = 'S';
        } else {
            rcase = 'r';
            event_s = 's';
        }

        packet_message(rcase, packet.type, packet.number, packet.info);

        //deal with received packet
        switch(packet.type) {
            case FIN:
                fin++;
                pack = snprintf(buffer, 1024, "Magic: CSc361\nType: ACK\nAcknowledgement: %u\nWindow: %u\n\n",
                    seq_no+1, window);
                numbytes = sendto(sockfd, buffer, pack, 0, (struct sockaddr *)
                    &sender_addr, sender_len);

                if(numbytes < 0) {
                    perror("sendto");
                    exit(-1);
                }
                ack++;
                packet_message(event_s, ACK, seq_no+1, window);
                return 0;
            case DAT:
                // Check if DAT packet was expected.
                if (packet.number == seq_no) {
					if(packet.info<1024){
						pack = packet.info;
					}
					else{
						pack = 1024;
					}
                   
                    memcpy(data + *read, packet.data, pack);
                    *read += pack;
                    seq_no += pack;
                    window -= pack;
                    
                   
                }

                
             
                break;
            case SYN:
                syn++;
                break;
            case RST:
                rrst++;
                rcase = 'r';
                return -1;
        }

        bzero(buffer, strlen(buffer));

        // Acknowledge packet.
	window += pack;
        pack = snprintf(buffer, 1024, "Magic: CSc361\nType: ACK\nAcknowledgement: %u\nWindow: %u\n\n", seq_no, window);
        numbytes = sendto(sockfd, buffer, pack, 0, (struct sockaddr *)
            &sender_addr, sender_len);


        ack++;
        packet_message(event_s, ACK, seq_no, window);
    }

    return 1;

}

int connection_r() {

    char buffer[1024];
    struct packetheader packet;
    int pack, numbytes;
    char *token;

    numbytes = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)
        &(sender_addr), &sender_len);
    

    if(numbytes < 0) {
        perror("recvfrom");
        return -1;
    }

    //read from packet
    token = strtok(buffer, " :\t\n");
    token = strtok(NULL, " :\t\n");
    if(strcmp(token, "CSc361") != 0) {
        perror("invalid packet");
        return -1;
    }
		for(z=0; z<2; z++){
			token = strtok(NULL, " :\t\n");
		}
    //check the type of the packet
    if(strcmp(token, "RST") == 0) {
        packet.type = RST;
    } 
    else if(strcmp(token, "SYN") == 0) {
        packet.type = SYN;
        token = strtok(NULL, " :\t\n");
        token = strtok(NULL, " :\t\n");
        packet.number = atoi(token);
    }
    else if(strcmp(token, "FIN") == 0) {
	packet.type = FIN;
		for(z=0; z<2; z++){
			token = strtok(NULL, " :\t\n");
		}
        packet.number = atoi(token);
    }
    else {
        perror("invalid packet");
        return -1;
    }

    bzero(buffer, strlen(buffer));

    packet_message('r', packet.type, packet.number, packet.info);

    //check if the packet is an syn packet
    if(packet.type != SYN) {
        switch(packet.type) {
            case FIN:
                fin++;
                break;
            case RST:
                rrst++;
        }
        fprintf(stderr, "Need SYN packet\n");
        return -1;
    } else {
        syn++;
    }

    seq_no = packet.number+1;
    window = WINDOW_SIZE;

    //send ack packet
    pack = snprintf(buffer, 1024, "Magic: CSc361\nType: ACK\nAcknowledgement: %u\nWindow: %u\n\n", seq_no, window);
    numbytes = sendto(sockfd, buffer, pack, 0, (struct sockaddr *)
        &sender_addr, sender_len);
    if(numbytes < 0) {
        perror("sendto");
        return -1;
    }
    ack++;
    packet_message('s', ACK, seq_no, window);
    return 0;
}

int main(int argc, char *argv[]) {

    char buffer[MAXBUFLEN];
    int fd, numbytes;
    size_t received;

	if(argc < 4) {
		printf("Usage: rdpr receiver_ip receiver_port sender_file_name\n");
		exit(0);
	}

	receiver_ip = argv[1];
	receiver_port = atoi(argv[2]);
	sender_file_name = argv[3];

    //open the given file
    fd = open(sender_file_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(fd < 0) {
        perror("open");
        exit(0);
    }

	//Set up connection
	//The first step: creating a socket of type of UDP
    //error checking for every function call is necessary!
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        close(fd);
       	perror("sws: error on socket()");
       	return -1;
    }

   //Create addr for sender
    bzero((char *) &sender_addr, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;   //Address family, for us always: AF_INET
    sender_addr.sin_addr.s_addr = INADDR_ANY;
    sender_addr.sin_port = htons(receiver_port);  //byte order again
    sender_len = sizeof(sender_addr);

    //Create addr for receiver
    bzero((char *) &receiver_addr, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;	//Address family, for us always: AF_INET
    receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip); //Listen on receiver ip
    receiver_addr.sin_port = htons(receiver_port);  //byte order again
    receiver_len = sizeof(receiver_addr);

    //Bind the socket with the address information: 
    if (bind(sockfd, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr)) < 0){
        close(fd);
        close(sockfd);
	  	perror("error on binding!");
	  	return -1;
    }

    //begin the timer
    //gettimeofday(&begin, NULL);

    //connect to sender
	clock_t starttime = clock(), diff;
    if(connection_r() < 0) {
        close(fd);
        close(sockfd);
        return -1;
    }

    //receive data from sender
    do {
        numbytes = rdp_receive(buffer, &received);

        if(write(fd, buffer, received) < 0) {
            perror("write");
            break;
        }
    } while(numbytes > 0);

    //end the timer
	
	
	diff = clock() - starttime;

	int msec = diff / CLOCKS_PER_SEC;


    
    printf("total time duration (second): %d s\n", msec);
	
	

    close(fd);

    close(sockfd);

	return 0;
}
