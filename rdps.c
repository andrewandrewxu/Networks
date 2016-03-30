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
#define DAT 0
#define ACK 1
#define SYN 2
#define FIN 3
#define RST 4
#define MAX_PAYLOAD_SIZE 1024
#define WINDOW_SIZE 10240

//CREATE the struct here to convenient manipulate stats 
struct packetheader {
	int type;
	int number;
	int info;
	char numberofchars;
	char thepacket_type;
	int countfor_chars;
} packetheader;

//Globals for connection
int sockfd;
struct sockaddr_in sender_addr;
struct sockaddr_in receiver_addr;

char *sender_file_name;
struct timeval begin, end;
int window;
int seq_no;
unsigned int sentbytes = 0;
unsigned int upkts = 0;
int syn = 0;
int ack = 0;
int z=0;
int rst = 0;
int countrst = 0;
int dat = 0;
char *sender_ip;
int sender_port;
char *receiver_ip;
int receiver_port;
int fin = 0;
unsigned int tfboys = 0;
char questionmark;
unsigned int allpacketss = 0;




void packet_message(char event, struct sockaddr_in *sender_addr, struct sockaddr_in *receiver_addr, int type, unsigned int number, unsigned int info);
//send fin packet to close the connection
int rdp_close(){
	char buffer[MAX_PAYLOAD_SIZE];
	char *token;
	struct packetheader packet;
	packet.info = 0;
	packet.number = 0;
	struct timeval timeout;
	int retry, pack, numbytes;
	fd_set read_fds;
	socklen_t receiver_len = sizeof(receiver_addr);

	//if FIn been sent without ACK, print error message
	for (retry = 0; retry < 3; retry++) {
		//send fin packet
		
		bzero(buffer, strlen(buffer));
		pack = snprintf(buffer, MAX_PAYLOAD_SIZE, "Magic: CSc361\nType: FIN\nSequence: %u\n\n", seq_no);
		numbytes = sendto(sockfd, buffer, pack, 0, (struct sockaddr *)
			&receiver_addr, receiver_len);

		bzero(buffer, strlen(buffer));

		
		fin++;
		//print the packet message
		if(retry){
			questionmark = 'S';
		}
		else{
			questionmark = 's';
		}
		packet_message(questionmark, &sender_addr,
			&receiver_addr, FIN, seq_no, 0);

		do{
			FD_ZERO(&read_fds);
			FD_SET(sockfd, &read_fds);

			//Timeout
			timeout.tv_sec = 0;
			timeout.tv_usec = 1000000;

			//select timeout
			numbytes = select(sockfd+1, &read_fds, NULL, NULL, &timeout);

			if(numbytes < 0) {
				perror("select");
				exit(-1);
			} else if(numbytes > 0) {
				//receive the packet from receiver
				numbytes = recvfrom(sockfd, buffer, MAX_PAYLOAD_SIZE, 0,
					(struct sockaddr *)&receiver_addr, &receiver_len);

				if(numbytes < 0) {
					perror("recvfrom");
					exit(-1);
				}

				//read from received packet
				for(z=0; z<2; z++){
					token = strtok(NULL, " :\t\n");
				}
			    if(strcmp(token, "CSc361") != 0) {
			    	perror("invalid packet");
			    	return -1;
			    }
				for(z=0; z<2; z++){
					token = strtok(NULL, " :\t\n");
				}
			    if(strcmp(token, "ACK") == 0) {
			    	packet.type = ACK;
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
			    } else {
			    	perror("invalid packet");
			    	return -1;
			    }

				bzero(buffer, strlen(buffer));

				if(packet.number < seq_no+1){
					questionmark = 'R';
				}
				else{
					questionmark = 'r';
				}
				packet_message(questionmark,
					&receiver_addr, &sender_addr, packet.type, packet.number, packet.info);

				//ACK packet
				if(packet.type == ACK) {
					ack++;

					if(packet.number == seq_no+1) {
						return 0;
					}
				}
				else if (packet.type == RST) {
					countrst++;
					return -1;
				} else {
					//if the received packet is not ack or rst, send rst packet to receiver
					pack = snprintf(buffer, MAX_PAYLOAD_SIZE, "Magic: CSc361\nType: RST\n\n");
					numbytes = sendto(sockfd, buffer, pack, 0,
						(struct sockaddr *)&receiver_addr, receiver_len);

					bzero(buffer, strlen(buffer));

					

					rst++;
					packet_message('s', &sender_addr, &receiver_addr,
						RST, 0, 0);
				}
			}
		} while(numbytes);
	}
	fprintf(stderr, "no response\n");
	return -1;
}

//send data packet
int packetize(char *data, size_t file_length){

	char buffer[MAX_PAYLOAD_SIZE];
	char *token;
	char event;
	struct packetheader packet;
	packet.info = 0;
	packet.number = 0;
	struct timeval timeout;
	size_t sent = 0;
	unsigned int pre = seq_no - 1;
	unsigned int received;
	unsigned int retry = 0;
	int pack, i, numbytes;
	fd_set read_fds;
	unsigned int send = file_length;
	unsigned int start = seq_no;

	unsigned int rmd, pay,seq;
	socklen_t receiver_len = sizeof(receiver_addr);

	//send packets while there are still have packets to be sent
	while(send) {
		//remaining data to be sent is the minimum of windowsize and unacknowleged data
		if(window < send){
			rmd = window;
		}
		else{
			rmd = send;
		}
		seq = seq_no;

		//send the packts
		for (i = 0; i < 100 && rmd; i++) {
			
			pay = rmd < 959 ? rmd : 959;

			//reduce remaining data
			rmd = rmd - pay;

			//send data
			pack = snprintf(buffer, MAX_PAYLOAD_SIZE, "Magic: CSc361\nType: DAT\nSequence: %u\nPayload %u\n\n", seq, pay);
			memcpy(buffer+pack, data+seq-start, pay);
			numbytes = sendto(sockfd, buffer, pack+pay, 0, (struct sockaddr *)&receiver_addr,
				receiver_len);

			//clear buffer
			bzero(buffer, strlen(buffer));

			//check if sequence number has been sent already
			if(seq > pre) {
				pre = seq;
				event = 's';
				tfboys += pay;
				sentbytes += pay;
				upkts++;
				allpacketss++;
			} else {
				event = 'S';
				tfboys += pay;
				allpacketss ++;
			}

			packet_message(event, &sender_addr, &receiver_addr, DAT, seq, pay);
			seq += pay;

		}

		received = 0;

		do {
			FD_ZERO(&read_fds);
			FD_SET(sockfd, &read_fds);

			timeout.tv_sec = 0;
			timeout.tv_usec = 200000;

			numbytes = select(sockfd+1, &read_fds, NULL, NULL, &timeout);

			if(numbytes < 0) {
				perror("select");
				exit(-1);
			} else if(numbytes > 0) {
				numbytes = recvfrom(sockfd, buffer, MAX_PAYLOAD_SIZE, 0, NULL, NULL);

				if(numbytes < 0) {
					perror("recvfrom");
					exit(-1);
				}

				received ++;

				//read from received packet
				for(z=0; z<2; z++){
					token = strtok(NULL, " :\t\n");
				}
    			if(strcmp(token, "CSc361") != 0) {
    				perror("invalid packet");
    				return -1;
    			}
				for(z=0; z<2; z++){
					token = strtok(NULL, " :\t\n");
				}
			    if(strcmp(token, "ACK") == 0) {
			    	packet.type = ACK;
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
			    } else {
			    	perror("invalid packet");
			    	return -1;
			    }

				bzero(buffer, strlen(buffer));

				if(packet.type == ACK) {
					//if it has unacked data, prepare to send new data
					if(packet.number > seq_no) {
						event = 'r';
						sent += packet.number - seq_no;
						seq_no = packet.number;
						window = packet.info;
						send = file_length - sent;
						if(packet.number == seq) {
							numbytes = 0;
						}
					} else {
						event = 'R'; 
					}

					ack++;
					packet_message(event, &receiver_addr, &sender_addr, packet.type,
						packet.number, packet.info);
				}

				else if(packet.type == RST) {
					countrst++;
					packet_message('r', &receiver_addr, &sender_addr,
						packet.type, packet.number, packet.info);
					return -1;
				}
			} 
		} while (numbytes);

		//if no packets are received, increase retry
		if(!received) {
			retry++;
		} else {
			retry = 0;
		}

		if(retry == 3) {
			//send rst packet
			pack = snprintf(buffer, MAX_PAYLOAD_SIZE, "Magic: CSc361\nType: RST\n\n");
			numbytes = sendto(sockfd, buffer, pack, 0, 
				(struct sockaddr *)&receiver_addr, receiver_len);
			bzero(buffer, strlen(buffer));
			if(numbytes < 0) {
				perror("sendto");
				exit(-1);
			}
			rst++;
			packet_message('s', &sender_addr, &receiver_addr, RST, 0, 0);
			gettimeofday(&begin, NULL);
			return -1;
		}
	}

	return 0;
	
}



void packet_message(char event, struct sockaddr_in *sender_addr, struct sockaddr_in *receiver_addr, int type, unsigned int number, unsigned int info) {

    char s_addr[16];
    char r_addr[16];
    struct timeval tbv;
    unsigned int h, mcm, sos, sus;

    //get current time
    gettimeofday(&tbv, NULL);
    h = (tbv.tv_sec/3600-8) % 24;
    mcm = tbv.tv_sec / 60 % 60;
    sos = tbv.tv_sec % 60;
    sus = tbv.tv_usec;

    //get sender and receiver IP addresses
    strncpy(s_addr, sender_ip, 16);
    strncpy(r_addr, receiver_ip, 16);

    //print packet info
    switch(type) {
        case ACK:
        	printf("%02u:%02u:%02u.%d %c %s:%d %s:%d ACK %u %u\n", h, mcm, sos, sus,
                event, s_addr, sender_port, r_addr, receiver_port,
                number, info);
        	break;
        case DAT:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d DAT %u %u\n", h, mcm, sos, sus,
                event, s_addr, sender_port, r_addr, receiver_port,
                number, info);
            break;
        case FIN:
        	printf("%02u:%02u:%02u.%d %c %s:%d %s:%d FIN %u 0\n", h, mcm, sos, sus,
                event, s_addr, sender_port, r_addr, receiver_port,
                number);
        	break;
        case SYN:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d SYN %u 0\n", h, mcm, sos, sus,
                event, s_addr, sender_port, r_addr, receiver_port,
                number);
            break;
        case RST:
            printf("%02u:%02u:%02u.%d %c %s:%d %s:%d RST\n", h, mcm, sos, sus,
                event, s_addr, sender_port, r_addr, receiver_port);
            break;
        default:
            fprintf(stderr, "invalid packet\n");    
    }
}

//send syn packet
int sentsyn() {
	struct packetheader packet;
	char buffer[MAX_PAYLOAD_SIZE];
	

    packet.number = 0;
    socklen_t receiver_len;
    fd_set read_fds;
	struct timeval timeout;
    
    packet.info = 0;
    receiver_len = sizeof(receiver_addr);
	int random = rand();
	char *token;
	int numbytes, pack, retry;
	seq_no = random;

	pack = snprintf(buffer, MAX_PAYLOAD_SIZE, "Magic: CSc361\nType: SYN\nSequence: %u\n\n", seq_no);

    for (retry = 0; retry < 3; retry++) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        timeout.tv_sec = 1000000*(1<<retry)/1000000;		// learned from friend
        timeout.tv_usec = 1000000*(1<<retry)%1000000;

    	//send syn packet
    	if ((numbytes = sendto(sockfd, buffer, pack, 0,
              (struct sockaddr *)&receiver_addr, receiver_len)) == -1) {
              perror("error in sendto()");
              return -1;
        }
        syn++;

		if(retry){
			questionmark = 'S';
		}
		else{
			questionmark = 's';
		}
        packet_message(questionmark, &sender_addr, &receiver_addr, SYN, seq_no, 0);

        numbytes = select(sockfd +1, &read_fds, NULL, NULL, &timeout);

        if(numbytes < 0) {
            perror("select");
            return -1;
        } else {
            break;
        }
    }

    bzero(buffer, strlen(buffer));

    if(retry >= 3) {
        fprintf(stderr, "timeout\n");
        return -1;
    }

    numbytes = recvfrom(sockfd, buffer, MAX_PAYLOAD_SIZE, 0, (struct sockaddr *)&receiver_addr, &receiver_len);
    if(numbytes < 0) {
        perror("recvfrom");
        return -1;
    }

    //read from received packet
    token = strtok(buffer, " :\t\n");
    token = strtok(NULL, " :\t\n");
    if(strcmp(token, "CSc361") != 0) {
    	perror("invalid packet");
    	return -1;
    }
    token = strtok(NULL, " :\t\n");
    token = strtok(NULL, " :\t\n");
    if(strcmp(token, "ACK") == 0) {
    	packet.type = ACK;
    	token = strtok(NULL, " :\t\n");
    	token = strtok(NULL, " :\t\n");
    	packet.number = atoi(token);
    	token = strtok(NULL, " :\t\n");
    	token = strtok(NULL, " :\t\n");
    	packet.info = atoi(token);
    }
    else if(strcmp(token, "RST") == 0) {
    	packet.type = RST;
    } else {
    	perror("invalid packet");
    	return -1;
    }

    bzero(buffer, strlen(buffer));

    packet_message('r', &receiver_addr, &sender_addr, packet.type, packet.number, packet.info);

    //check response
    switch(packet.type) {
        case ACK:
        ack++;
        if(packet.number == seq_no + 1) {
            seq_no ++;
            window = packet.info;
            return 0;
        }
        //if the received packet is not ack or rst, send rst packet to receiver
        default:
        pack = snprintf(buffer, MAX_PAYLOAD_SIZE, "Magic: CSc361\nType: RST\n\n");
        if ((numbytes = sendto(sockfd, buffer, pack, 0,
              (struct sockaddr *)&receiver_addr, receiver_len)) == -1) {
              perror("error in sendto()");
              return -1;
        }
        bzero(buffer, strlen(buffer));
        rst++;
        packet_message('s', &sender_addr, &receiver_addr, RST, 0, 0);
        case RST:
        rst++;
        fprintf(stderr, "fail to connect\n");
        return -1;
    }
}




int main(int argc, char *argv[]) {

	double dur;

	if(argc < 6) {
		printf("Usage: rdps sender_ip sender_port receiver_ip receiver_port sender_file_name\n");
		exit(0);
	}

    sender_ip = argv[1];
	sender_port = atoi(argv[2]);
	receiver_ip = argv[3];
	receiver_port = atoi(argv[4]);
	sender_file_name = argv[5];

	//Set up connection
	//The first step: creating a socket of type of UDP
    //error checking for every function call is necessary!
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("error on socket()");
        return -1;
    }

    //Create addr for sender
    bzero((char *) &sender_addr, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;	//Address family, for us always: AF_INET
    sender_addr.sin_addr.s_addr = inet_addr(sender_ip); //Listen on any ip address I have
    sender_addr.sin_port = htons(sender_port);  //byte order again
    bzero((char *) &receiver_addr, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;	//Address family, for us always: AF_INET
    receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip); //Listen on any ip address I have
    receiver_addr.sin_port = htons(receiver_port);  //byte order again

    //Bind the socket with the address information: 
    if (bind(sockfd, (struct sockaddr *) &sender_addr, sizeof(sender_addr)) < 0){
        close(sockfd);
	  	perror("error on binding!");
	  	return -1;
    }

    //start the time here
	clock_t starttime = clock(), diff;
    //send syn packet for connection
    sentsyn();
    
	int fd;
    char *mapped;
    struct stat sss;

    //Check file
	
	

    if ((fd = open(sender_file_name, O_RDONLY)) == -1) {
	perror("open");
       	exit(1);
    }

    if (stat(sender_file_name, &sss) == -1) {
    	perror("error occur");
        exit(1);
    }

    if(sss.st_size == 0) {
	//send data to receiver
   	 if(packetize(NULL, 0) >= 0) {

	rdp_close();
   	}
    } else {
    	mapped = mmap((caddr_t)0, sss.st_size, PROT_READ, MAP_SHARED, fd, 0);
    

    	mapped == (caddr_t)(-1); 
			
   	//send data to receiver
   	 if(packetize(mapped, sss.st_size) >= 0) {

	rdp_close();
   	}
    }
	
	
	
	

    //end the timer
    //gettimeofday(&end, NULL);
    //dur = end.tv_sec - begin.tv_sec;
    //dur += (end.tv_usec - begin.tv_usec) / 1000000.0;
	
	diff = clock() - starttime;

	int msec = diff / CLOCKS_PER_SEC;

    //rdp_message(dur);
	printf("total data bytes sent: %u\n", tfboys);
	printf("unique data bytes sent: %u\n", sentbytes);
	printf("total data packets sent: %u\n", allpacketss);
	printf("unique data packets sent: %u\n", upkts);
	printf("SYN packets sent: %u\n", syn);
	printf("FIN packets sent: %u\n", fin);
	printf("RST packets sent: %u\n", rst);
	printf("ACK packets received: %u\n", ack);
	printf("RST packets received: %u\n", countrst);
	printf("total time duration (second): %d s\n", msec);

    close(sockfd);

	return 0;
}
