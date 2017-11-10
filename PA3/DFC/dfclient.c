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


int create_sockets(){
 	int i;
 	for(i = 0; i < NO_OF_CONNECTIONS; i++){
 		if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) <0) {
  			perror("Problem in creating the socket");
  			return 1;
 		}	
 	}
 	return 0;
}

int main(int argc, char **argv){

	//basic check for the arguments
 	if (argc != 2) {
  		printf("Usage: dfclient <conf file>\n"); 
  		exit(1);
 	}

 	//creating multiple sockets
 	/*if(create_sockets()){
 		printf("Sockets not created. Exiting...\n");
 		exit(1);
 	}

 	//Create a socket for the client
 	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  		perror("Problem in creating the socket");
  		exit(1);
 	}*/

 	//opening the conf file
	FILE *fp = fopen(argv[1], "r");
	printf("Parsing the client configuration file ...\n");
	parse_conf(fp);
	fclose(fp);

	//Creation of the socket
 	/*memset(&servaddr, 0, sizeof(servaddr));
 	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr= inet_addr(argv[1]);
 	servaddr.sin_port =  htons(SERV_PORT); //convert to big-endian order
*/

	printf("\n-------------------------------------");
	printf("\nEnter one of these options:\n");
	printf("LIST\n");	
	printf("PUT [file_name]\n");	
	printf("GET [file_name]\n");	

	//getting input from user
	fgets(options, 20, stdin);

	/*while(1){
		if  (!strncmp(options, "LIST ", 5))
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
					break;
			//PUT
			case 1:
					break;
			//GET
			case 2:
					break;
			//default
			case 3:
					printf("Enter option correctly.\n");		
		}
	}*/


	printf("Folder 1 is %s\n", folder[0]);
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
	printf("Parsing complete.\n\n");

	return 0;
}