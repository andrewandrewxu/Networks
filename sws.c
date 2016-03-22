/* A simple server */
#include <sys/types.h>    
#include <stdio.h>        
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>       
#include <stdlib.h>      
#include <pthread.h> 
#include <termios.h>  
#include <unistd.h>       
#include <errno.h>

#define MAXBUFLEN 256


void *quiting() {
  int key;

  /* --- BEGIN DERIVED CODE :: Found http://stackoverflow.com/questions/2984307/c-key-pressed-in-linux-console/2984565#2984565 */
  struct termios org_opts, new_opts;
  int res=0;
      /*-----  store old settings -----------*/
  res=tcgetattr(STDIN_FILENO, &org_opts);
      /*---- set new terminal parms --------*/
  memcpy(&new_opts, &org_opts, sizeof(new_opts));
  new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ICRNL);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
  do {                  /* CHANGED */
    key=getchar();      /* CHANGED */
  } while(key != 113);  /* CHANGED */
      /*------  restore old settings ---------*/
  res=tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
  /* --- END DERIVED CODE */
  exit(0);
  return((void *) 0); 
}


int main(int argc, char *argv[]){
     //Using socket() function for communication
     //int socket(int domain, int type, int protocol);
     //domain is PF_INET, type is SOCK_DGRAM for UDP, and protocol can be set to 0 to choose the proper protocol!
     int sockfd, portno;
     socklen_t cli_len;
     char buffer[MAXBUFLEN];	//data buffer
     struct sockaddr_in serv_addr, cli_addr;	//we need these structures to store socket info
     int numbytes;
	char* send_port;

     if (argc < 2) {
         printf( "Usage: %s <port>\n", argv[0] );
         fprintf(stderr,"ERROR, no port provided\n");
         return -1;;
     }

     //The first step: creating a socket of type of UDP
     //error checking for every function call is necessary!
     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
	  perror("sws: error on socket()");
	  return -1;
     }

     /* prepare the socket address information for server side:
      (IPv4 only--see struct sockaddr_in6 for IPv6)

    struct sockaddr_in {
	short int          sin_family;  // Address family, AF_INET
	unsigned short int sin_port;    // Port number
	struct in_addr     sin_addr;    // Internet address
	unsigned char      sin_zero[8]; // Same size as struct sockaddr
     };
     */
     /*Clear the used structure by using either memset():
     memset(&serv_addr, 0, sizeof(struct sockaddr_in));	     or bzero(): */
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);  //read the port number value from stdin
	send_port = argv[1];
     serv_addr.sin_family = AF_INET;	//Address family, for us always: AF_INET
     //serv_addr.sin_addr.s_addr = inet_addr("142.104.69.255");
     //serv_addr.sin_addr.s_addr = inet_addr("10.10.1.100"); //INADDR_ANY; //Listen on any ip address I have
     serv_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY; //Listen on any ip address I have
     serv_addr.sin_port = htons(portno);  //byte order again 

     //Bind the socket with the address information: 
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0){
          close(sockfd);
	  perror("sws: error on binding!");
	  return -1;
     }


	char* change_dir = argv[2];
	chdir(change_dir);

	//if(chdir(change_dir)==-1){
	//	printf("failed to enter the directory\n");
	//}
	printf("sws is running on UDP port %d and serving %s \n", portno, change_dir);
	printf("press q to quit...\n");
	
	char dir[256];
	getcwd(dir, 256);
	

	pthread_t quit_thread;
  	pthread_create(&quit_thread, 0, quiting, 0);


*/
while(1){



     //printf("sws: waiting to recvfrom...\n");
     cli_len = sizeof(cli_addr);
     //the sender address information goes to cli_addr
     if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN-1 , 0,
         (struct sockaddr *)&cli_addr, &cli_len)) == -1) {
         perror("sws: error on recvfrom()!");
         return -1;
     }

				//set the current time

    	
	//find the path { GET / HTTP/1.0 }
	int index=0, space=0, j=0, k=0;
	char path[256];
	char http_check[256];
	bzero(path, 256);
	//char* p;
	char* message;
	long fsize = 0;
	char* fcontent;


	while(index<3){
		toupper(buffer[index]);
		index++;
	}
	index = 0;


	if(strncmp(buffer, "GET", 3)!=0){
		message = "HTTP/1.0 400 Bad Request;  ";
		sendto(sockfd, message, strlen(message), 0,
             (struct sockaddr *)&cli_addr, cli_len);
	}
	else{
		space = 4;
		while(buffer[space]!=' '){
			path[index] = buffer[space];	//put the path part in to the array path
			index++;
			space++;
		}
		//check if the http version is correct
		space++; 		//move to http part
		while(buffer[space]!=' '){
			http_check[j] = buffer[space];
			j++;
			space++;
		}
		

		path[index] = '\0'; //set the last char to null	
		if((strncmp(path, "/", 1) != 0) || (strncmp(http_check, "HTTP/1.0", 8) != 0)){
			message = "HTTP/1.0 400 Bad Request;  ";
			sendto(sockfd, message, strlen(message), 0,
             (struct sockaddr *)&cli_addr, cli_len);
		}
		else if(strncmp(path, "/../", 4) == 0){
			message = "HTTP/1.0 404 Not Found;  ";
			sendto(sockfd, message, strlen(message), 0,
             (struct sockaddr *)&cli_addr, cli_len);
		}
		else{
			char* full_path = strcat(dir, path);	// add the path to the directory and assign to path, so path now has the full path	
			
			if (full_path[strlen(full_path)-1] == '/') {
      				strncat(full_path, "index.html", 12);
  			}
			
			//check if we can read the file 
			if(access(full_path, R_OK)<0){
				message = "HTTP/1.0 404 NOt Found;  ";
				sendto(sockfd, message, strlen(message), 0,
             (struct sockaddr *)&cli_addr, cli_len);
			}
			else{
				int c;
				FILE* fp = fopen(full_path, "r");
				printf("%s\n ", full_path);
				if (fp==NULL){
					message = "HTTP/1.0 404 Not Found;  ";
					sendto(sockfd, message, strlen(message), 0,
             (struct sockaddr *)&cli_addr, cli_len);
				}
				
				// read the file here
				else{
					
					
					fseek (fp , 0 , SEEK_END);
					fsize = ftell (fp);
					fseek (fp, 0, SEEK_SET);

					fcontent = malloc(fsize+1);

					
				
					fread(fcontent, fsize, sizeof(fcontent), fp);
					printf(" %s\n ",fcontent);
   				 	
					fclose(fp);
					fcontent[fsize-1] = '\n'; 
					fcontent[fsize] = '\0';
					message = "HTTP/1.0 200 OK;  ";
					sendto(sockfd, message, strlen(message), 0,
             (struct sockaddr *)&cli_addr, cli_len);
					
					//modify full_path a bit to fit the format
					strncat(full_path, "; \n", 3);
					sendto(sockfd, full_path, strlen(full_path), 0,
             (struct sockaddr *)&cli_addr, cli_len);
					sendto(sockfd, fcontent, strlen(fcontent), 0,
             (struct sockaddr *)&cli_addr, cli_len);
				}
			}
			
			
		}
	}


	time_t current_time;				// reference time part from stackoverflow
	struct tm * mytime;
	time(&current_time);						//dealing with time here
	char* t;
	char ti[256];
	int i=0;
	mytime = localtime(&current_time);
  	strcpy(t, asctime(mytime));
	
	  for(i=1; i<strlen(t); i++) {
    	if(t[i] == '\n') {
     	 t[i] = ' ';
   	 }
  	}
 	 strncpy(ti, &t[4], 15);
	strncat(ti, ";  ", 3);
	sendto(sockfd, ti, strlen(ti), 0,
             (struct sockaddr *)&cli_addr, cli_len);
	strncat(inet_ntoa(cli_addr.sin_addr), ";", 1);
	sendto(sockfd, inet_ntoa(cli_addr.sin_addr), strlen(inet_ntoa(cli_addr.sin_addr)), 0,
             (struct sockaddr *)&cli_addr, cli_len);
	
	strncat(send_port, "  ", 2);
	sendto(sockfd, send_port, strlen(send_port), 0,
             (struct sockaddr *)&cli_addr, cli_len);

	
	printf("%s %s %s:%d\n", message, ti, inet_ntoa(cli_addr.sin_addr), portno );


	


    //printf("sws: received packet from IP: %s and Port: %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    //printf("listener: received packet is %d bytes long\n", numbytes);
    buffer[numbytes] = '\0';
    //printf("listener: packet contains \"%s\" \n", buffer);

    if ((numbytes = sendto(sockfd, buffer, strlen(buffer), 0,
             (struct sockaddr *)&cli_addr, cli_len)) == -1) {
            perror("sws: error in sendto()");
            return -1;
    }
    }	
    close(sockfd);
	
		
	
	
    //freeing memory!
    return 0;
}
