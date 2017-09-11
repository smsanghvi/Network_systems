#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>


#define MAXBUFSIZE 22500

int main (int argc, char * argv[] )
{
	int sock;   //socket descriptor
	struct addrinfo s_addr, *s_info, *temp;
	struct sockaddr_storage remote;
	int a;      //return value of getaddrinfo
	int nbytes; 
	char buffer[MAXBUFSIZE];
	socklen_t remote_len;
	FILE *fp;
	
	if(argc != 2){
		printf ("USAGE: <port>\n");
		exit(1);
	}
	else if( atoi(argv[1]) < 5000 ){	
		printf("Enter <port> greater than 5000\n");
		exit(1);
	}
	
			

	//packing the s_addr addrinfo structure
	memset(&s_addr, 0, sizeof s_addr);
	s_addr.ai_flags = AI_PASSIVE;  //to use its own IP
	s_addr.ai_family = AF_INET; //only defining for IPv4 addresses
	s_addr.ai_socktype = SOCK_DGRAM;  //setting it to type datagram
	s_addr.ai_protocol = IPPROTO_UDP;
	
	if ((a = getaddrinfo(NULL, argv[1], &s_addr, &s_info)) != 0){
		perror("getaddrinfo error.\n");
		return 1;
	}

	//looping through the instances of the created linked list
	for(temp = s_info; temp != NULL; temp = temp->ai_next){
		if((sock = socket(PF_INET, SOCK_DGRAM, 0))== -1){
			perror("Socket not setup on server.\n");
			continue;			
		}

		printf("Socket created succesfully on server end.\n");

		if( bind(sock, temp->ai_addr, temp->ai_addrlen) == -1){
			perror("Cannot bind port on server.\n");		
			continue;
		}

		printf("Port %s successfully attached to socket.\n", argv[1]);
		break;
	}
		
	if( temp == NULL ){
		perror("Failed to create socket and/or bind port to it.\n");
		return 3;
	}

	//done with s_info, lets free the space
	freeaddrinfo(s_info);

	printf("Waiting to receive data ...\n"); 

	remote_len = sizeof remote;
	fp = fopen("foo5.png","w+");
	if ((nbytes = recvfrom(sock, buffer, sizeof buffer, 0,  \
		(struct sockaddr *)&remote, &remote_len	)) < 0){
		perror("Error in recvfrom on server.\n");
		exit(1);
	}
	
	if(fwrite(buffer, nbytes, 1, fp) <= 0){
		printf("Unable to write to file.\n");
		exit(1);
	}

	fclose(fp);

	printf("Number of bytes received is %d.\n", nbytes);
	buffer[nbytes] = '\0';
	//printf("Packet received is %s\n", buffer);

	//closing socket connection
	close(sock);

	return 0;
}
