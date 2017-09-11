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
#include <errno.h>

#define MAXBUFSIZE 22500

int main (int argc, char * argv[]){

	int sock;   //socket descriptor
	struct addrinfo s_addr, *s_info, *temp;
	int a;
	int nbytes;
	int16_t menu_id = -1;	
	char options[20];
	char buffer[MAXBUFSIZE] = "Hello world!";
	FILE *fp;

	if (argc != 3)
	{
		printf("USAGE: <server_ip> <server_port>\n");
		exit(1);
	}
	else if( atoi(argv[2]) < 5000 ){	
		printf("Enter <port> greater than 5000\n");
		exit(1);
	}


	memset(&s_addr, 0, sizeof s_addr);
	s_addr.ai_flags = AI_PASSIVE;  //to use its own IP
	s_addr.ai_family = AF_INET; //only defining for IPv4 addresses
	s_addr.ai_socktype = SOCK_DGRAM;  //setting it to type datagram
	s_addr.ai_protocol = IPPROTO_UDP;
	
	if ((a = getaddrinfo(argv[1], argv[2], &s_addr, &s_info)) != 0){
		perror("getaddrinfo error.\n");
		return 1;
	}

	//looping through the instances of the created linked list
	for(temp = s_info; temp != NULL; temp = temp->ai_next){
		if((sock = socket(PF_INET, SOCK_DGRAM, 0))== -1){
			perror("Socket not setup on client.\n");
			continue;			
		}

		printf("Socket created succesfully on client end.\n");
		break;
	}
		
	if( temp == NULL ){
		perror("Failed to create socket.\n");
		return 3;
	}

	//done with s_info, lets free the space
	freeaddrinfo(s_info);


	printf("\nPlease enter one of these options:\n");
	printf("get [file_name]\n");	
	printf("put [file_name]\n");	
	printf("delete [file_name]\n");	
	printf("ls\n");	
	printf("exit\n");	

	fgets(options, 20, stdin);
	if  (!strncmp(options, "get ", 4))
		menu_id = 0;
	else if (!strncmp(options, "put ", 4))
		menu_id = 1;
	else if (!strncmp(options, "delete ", 7))
		menu_id = 2;
	else if (!strncmp(options, "ls", 2))
		menu_id = 3;
	else if (!strncmp(options, "exit", 4))
		menu_id = 4;
	else
		menu_id = 5;


	switch(menu_id){
		case 0: 
			printf("\nGet\n");
			break; 
		case 1: 
			printf("\nPut\n");
			fp = fopen("foo5.png", "r");
			if(fp == NULL)
				printf("File does not exist.\n");
			
			else{
				size_t newLen = fread(buffer, sizeof(char), MAXBUFSIZE, fp);
				if(ferror(fp)!=0)
  					fputs("Error reading file", stderr);

				else{
					buffer[newLen++] = '\0';
				}

				fclose(fp);
			}

			if ((nbytes = sendto(sock, buffer, strlen(buffer), 0,  \
		(struct sockaddr *)temp->ai_addr, temp->ai_addrlen)) < 0){
			perror("Error in sending data from client end.\n");
			exit(1);
	}

	printf("Transmitted %d bytes\n", nbytes);
			break; 
		case 2: 
			printf("\nDelete\n");
			break; 
		case 3: 
			printf("\nls\n");
			break; 
		case 4: 
			printf("\nexit\n");
			break; 
		default:
			printf("\nEnter option correctly.\n");
			exit(1);

	}


	close(sock);
	return 0;

}
