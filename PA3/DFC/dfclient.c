#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
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

#define MAX_CONF_SIZE 1000
#define BUFFER_SIZE 1000
#define NO_OF_CONNECTIONS 4

static char folder[NO_OF_CONNECTIONS][10];
static char ip[NO_OF_CONNECTIONS][20];
int port[NO_OF_CONNECTIONS];
char username[20];
char password[20];
char options[20];
char *str;
int16_t menu_id = -1;	
int count = 0;
int sockfd[NO_OF_CONNECTIONS];
struct sockaddr_in servaddr[NO_OF_CONNECTIONS];


char buf_conf[MAX_CONF_SIZE];

void parse_conf(FILE *fp){
	while(count<6){
		//reading in a line at a time
		fgets(buf_conf, MAX_CONF_SIZE, fp);
		str = strtok(buf_conf, " ");
		switch(count){
			case 0:	str = strtok(NULL, " ");
					strcpy(folder[0], str);
					str = strtok(NULL, ":");
					strcpy(ip[0], str);
					port[0] = atoi(strtok(NULL, " "));
					break;
			case 1:	str = strtok(NULL, " ");
					strcpy(folder[1], str);
					str = strtok(NULL, ":");
					strcpy(ip[1], str);
					port[1] = atoi(strtok(NULL, " "));
					break;
			case 2:	str = strtok(NULL, " ");
					strcpy(folder[2], str);
					str = strtok(NULL, ":");
					strcpy(ip[2], str);
					port[2] = atoi(strtok(NULL, " "));
					break;
			case 3:	str = strtok(NULL, " ");
					strcpy(folder[3], str);
					str = strtok(NULL, ":");
					strcpy(ip[3], str);
					port[3] = atoi(strtok(NULL, " "));
					break;					
			case 4: str = strtok(NULL, " \n");
					strcpy(username, str);
					break;
			case 5: str = strtok(NULL, " \n");
					strcpy(password, str);
					break;
		}
		++count;
	}
}


//creating the 4 sockets, populating them and connecting to server
int create_sockets(){
	int i;
 	for(i = 0; i < NO_OF_CONNECTIONS; i++){
 		//making socket function calls
 		if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
  			perror("Problem in creating the socket");
  			return 1;
 		}	

 		//populating the structures
 		memset(&servaddr[i], 0, sizeof(servaddr[i]));
 		servaddr[i].sin_family = AF_INET;
		servaddr[i].sin_addr.s_addr= inet_addr(ip[i]);
 		servaddr[i].sin_port =  htons(port[i]); //convert to big-endian order

 		//Connection of the client to the socket 
 		if (connect(sockfd[i], (struct sockaddr *) &servaddr[i], sizeof(servaddr[i])) == -1) {
  			perror("Problem in connecting to the server");
  			return 1;
 		}
 	}

 	return 0;
}


int main(int argc, char **argv){
	int i;
	char buffer[BUFFER_SIZE];
	int bytes_read[4];
 	char resp[4][100];

	//basic check for the arguments
 	if (argc != 2) {
  		printf("Usage: dfclient <conf file>\n"); 
  		exit(1);
 	}

 	//opening the conf file
	FILE *fp = fopen(argv[1], "r");
	printf("Parsing the client configuration file...\n");
	parse_conf(fp);
	fclose(fp);

 	//creating multiple sockets
 	if(create_sockets()){
 		printf("Sockets not created. Exiting...\n");
 		exit(1);
 	}


	printf("\n-------------------------------------");
	printf("\nEnter one of these options:\n");
	printf("LIST\n");	
	printf("PUT [file_name]\n");	
	printf("GET [file_name]\n");	

	//getting input from user
	fgets(options, 20, stdin);

	while(1){
		if  (!strncmp(options, "LIST", 4))
			menu_id = 0;
		else if (!strncmp(options, "PUT ", 4))
			menu_id = 1;
		else if (!strncmp(options, "GET ", 4))
			menu_id = 2;
		else
			menu_id = 3;		

		switch(menu_id){
			//LIST
			case 0:
					printf("It is LIST option.\n");
					break;
			//PUT
			case 1:
					printf("It is PUT option.\n");
					//sending out user credentials
					for(i=0; i<NO_OF_CONNECTIONS; i++){					
						send(sockfd[i], username, strlen(username), 0);
					}
					for(i=0; i<NO_OF_CONNECTIONS; i++){					
						send(sockfd[i], "\n", 2, 0);
					}
					for(i=0; i<NO_OF_CONNECTIONS; i++){					
						send(sockfd[i], password, strlen(password), 0);
					}
					//waiting for correct credentials reply from servers
					for(i=0; i<NO_OF_CONNECTIONS; i++){					
						while((bytes_read[i] = recv(sockfd[i], resp[i], BUFFER_SIZE, 0)) > 0){}	
						if(!strcmp(resp[i], "Authentication successful!"))
							printf("Authentication successful with server %d!\n", i);
						else{
							printf("Authentication failed with server %d\n", i);
							exit(1);
						}
					}


					/*for(i=0; i<NO_OF_CONNECTIONS; i++){
						//sending out the user entered options
						send(sockfd[i], options, strlen(options), 0);
						while((file_bytes = fread(buffer_data, sizeof(char), BUF_MAX_SIZE, fp)) > 0){
							send(sock_connect, buffer_data, file_bytes, 0);
						}
					}*/
					break;
			//GET
			case 2:
					printf("It is GET option.\n");
					break;
			//default
			case 3:
					printf("Enter option correctly.\n");		
		}

		break;
	}

	//verbose file parsing
	/*printf("Folder 1 is %s\n", folder[0]);
	printf("Folder 2 is %s\n", folder[1]);
	printf("Folder 3 is %s\n", folder[2]);
	printf("Folder 4 is %s\n", folder[3]);
	printf("IP 1 is %s\n", ip[0]);
	printf("IP 2 is %s\n", ip[1]);
	printf("IP 3 is %s\n", ip[2]);
	printf("IP 4 is %s\n", ip[3]);
	printf("Port 1 is %d\n", port[0]);
	printf("Port 2 is %d\n", port[1]);
	printf("Port 3 is %d\n", port[2]);
	printf("Port 4 is %d\n", port[3]);
	printf("Username is %s\n", username);
	printf("Password is %s\n", password);
	printf("Parsing complete.\n\n");*/

	return 0;
}