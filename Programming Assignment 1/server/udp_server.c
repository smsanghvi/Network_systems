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
	struct sockaddr_in local, remote;
	int nbytes; 
	char buffer[MAXBUFSIZE];
	char temp2[1000];
	char path[1000]; //used in case of ls
	int16_t menu_id = -1;	
	socklen_t remote_len;
	size_t newLen;
	char *str, *str1;
	FILE *fp;
	FILE *fp2;
  	FILE *fp1;
	
	if(argc != 2){
		printf ("USAGE: <port>\n");
		exit(1);
	}
	else if( atoi(argv[1]) < 5000 ){	
		printf("Enter <port> greater than 5000\n");
		exit(1);
	}
	
			
	//packing the local structure
	memset(&local, 0, sizeof local);
	local.sin_family = AF_INET;  		//setting IPv4 family
	local.sin_port = htons(atoi(argv[1]));  //setting port number
	local.sin_addr.s_addr = INADDR_ANY;     //setting the address
	

	if((sock = socket(AF_INET, SOCK_DGRAM, 0))== -1)
		perror("Socket not setup on server.\n");

	printf("Socket created succesfully on server end.\n");

	if( bind(sock, (struct sockaddr *)&local, sizeof(local)) == -1)
		perror("Cannot bind port on server.\n");		

	printf("Port %s successfully attached to socket.\n", argv[1]);

	remote_len = sizeof remote;

	while(1){

	printf("\nWaiting to receive data ...\n"); 

	if ((nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0,  \
		(struct sockaddr *)&remote, &remote_len	)) < 0){
		perror("Error in recvfrom on server.\n");
		exit(1);
	}
	
	buffer[nbytes] = '\0';	


	if  (!strncmp(buffer, "get ", 4))
		menu_id = 0;
	else if (!strncmp(buffer, "put ", 4))
		menu_id = 1;
	else if (!strncmp(buffer, "delete ", 7))
		menu_id = 2;
	else if (!strncmp(buffer, "ls", 2))
		menu_id = 3;
	else if (!strncmp(buffer, "exit", 4))
		menu_id = 4;
	else
		menu_id = 5;

	switch(menu_id){
		case 0: 
			str = strtok (buffer," ");
  			str1 = strtok (NULL, " ");

			if( access( str1, F_OK ) != -1 ) {
    				if ((nbytes = sendto(sock, "200\n", 3, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
					perror("Error in sending data from client end.\n");
					exit(1);
				}
	
			fp = fopen(str1, "r"); 
			if(fp == NULL){
				printf("Requested file does not exist.\n\n");
				exit(1);
			}			

			else{
				newLen = fread(buffer, sizeof(char), MAXBUFSIZE, fp);
				if(ferror(fp)!=0)
  					fputs("Error reading file", stderr);

				else{
					buffer[newLen++] = '\0';
				}

				fclose(fp);
			}
			
			if ((nbytes = sendto(sock, buffer, newLen-1, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
				perror("Error in sending data from client end.\n");
				exit(1);
			}
			
			printf("Transmitted %d bytes\n", nbytes);


			}

 			else {
    				printf("Requested file does not exist\n");
				if ((nbytes = sendto(sock, "400\n", 3, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
					perror("Error in sending data from client end.\n");
					exit(1);
				}
			}

			break; 

		case 1: 
			str = strtok (buffer," ");
  			str1 = strtok (NULL, " ");
			
			if(!strncmp(str1, "foo1", 4))
				fp = fopen("foo1", "w+"); 
			else if(!strncmp(str1, "foo2", 4))
				fp = fopen("foo2", "w+"); 
			else if(!strncmp(str1, "foo3", 4))
				fp = fopen("foo3", "w+"); 

			if ((nbytes = sendto(sock, "200\n", 3, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
				perror("Error in sending data from client end.\n");
				exit(1);
			}

			//checking server response
			if((nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				exit(1);
			}


			if(fwrite(buffer, nbytes, 1, fp) <= 0){
				printf("Unable to write to file.\n");
				exit(1);
			}

			fclose(fp);

			printf("\nNumber of bytes received is %d.\n", nbytes);
			buffer[nbytes] = '\0';

			break; 

		case 2: 
			str = strtok (buffer," ");
  			str1 = strtok (NULL, " ");
			if( access( str1, F_OK ) != -1 ) {
				if(!remove(str1)){
					if ((nbytes = sendto(sock, "200\n", 3, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
						perror("Error in sending data from client end.\n");
						exit(1);
					}
					printf("\nFile deleted successfully!\n");				
				}
				else{
					if ((nbytes = sendto(sock, "400\n", 3, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
						perror("Error in sending data from client end.\n");
						exit(1);
					}
					printf("\nFile could not be deleted.\n");	
				}

			}
			else{
				printf("\nFile does not exist.\n"); 
				if ((nbytes = sendto(sock, "600\n", 3, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
						perror("Error in sending data from client end.\n");
						exit(1);
				}
			}

			break; 
		case 3: 
			
  			/* Open the command for reading. */
  			fp2 = popen("/bin/ls", "r");
  			if (fp2 == NULL) {
   				printf("Failed to run command\n" );
    				exit(1);
 			}
  			int i = 0;
  			fp1 = fopen("temp", "w+"); 
			
  			/* Read the output a line at a time - output it. */
  			while (fgets(path, sizeof(path), fp2) != NULL) {
    				//filtering out useless stuff
    				if(strlen(path)==1)
					continue;

    				if(fwrite(path, strlen(path), 1, fp1) <= 0){
					printf("Unable to write to file.\n");
					exit(1);
    				}
   
  			}
			fclose(fp1);
		
			//data written to a file 'temp'
			fp = fopen("temp", "r"); 
			if(fp == NULL){
				printf("Requested file does not exist.\n\n");
				exit(1);
			}			

			else{

				newLen = fread(buffer, sizeof(char), MAXBUFSIZE, fp);
				if(ferror(fp)!=0)
  					fputs("Error reading file", stderr);

				else{
					buffer[newLen++] = '\0';
				}

				fclose(fp);
			}
			
			//transmitting the contents of the file 'temp'
			if ((nbytes = sendto(sock, buffer, newLen-1, 0,  (struct sockaddr *)&remote, sizeof remote)) < 0){
				perror("Error in sending data from client end.\n");
				exit(1);
			}
			
			printf("Transmitted %d bytes\n", nbytes);  
  			pclose(fp2);
			remove("temp");
			
			break; 
		case 4: 
			printf("Server shutting down...\n");
			close(sock);
			exit(0); 
		default:
			printf("\nEnter option correctly.\n");
			exit(1);

	}//end of switch case
	

	}//end of while loop

	//closing socket connection
	//close(sock);


	
	return 0;
}

