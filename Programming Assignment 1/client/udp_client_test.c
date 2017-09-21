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

//max packet size for a particular interface is 576 bytes
//hence 8 + (1 * 568) = 568
#define MAXBUFSIZE 1024

typedef struct{
int index;
char data[MAXBUFSIZE];
int data_length;
}packet;


int main (int argc, char * argv[]){

	int sock;   //socket descriptor
	packet *pckt = NULL;
	long int total_size = 0;

	struct sockaddr_in local, remote;
	struct hostent *temp;
	socklen_t remote_len;
	int nbytes;
	int16_t menu_id = -1;	
	char options[20];
	char buffer[MAXBUFSIZE];
	size_t newLen;
	char *str, *str1;
	char *temp1 = "foo1";
	FILE *fp;
	remote_len = sizeof remote;

	if (argc != 3)
	{
		printf("USAGE: <server_ip> <server_port>\n");
		exit(1);
	}
	else if( atoi(argv[2]) < 5000 ){	
		printf("Enter <port> greater than 5000\n");
		exit(1);
	}

	temp = gethostbyname(argv[1]);
	if(!temp)
		perror("No such host.");

	if((sock = socket(AF_INET, SOCK_DGRAM, 0))== -1)
		perror("Socket not setup on client.\n");

	
	// building up the structure data
	memcpy((char *)&local.sin_addr, (char *)temp -> h_addr, temp->h_length);
	local.sin_family = AF_INET; //only defining for IPv4 addresses
	local.sin_port = htons(atoi(argv[2])); 	//setting port no. 


	printf("\n-----------------------------------");
	printf("\nPlease enter one of these options:\n");
	printf("get [file_name]\n");	
	printf("put [file_name]\n");	
	printf("delete [file_name]\n");	
	printf("ls\n");	
	printf("exit\n\n");	

	fgets(options, 20, stdin);

	while(1){

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
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				exit(1);
			}
			
			str = strtok (options," ");
  			str1 = strtok (NULL, " ");


			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}

			if(!strcmp(buffer, "200")){
				printf("\nServer acknowledged request for the file.\n");


			if((nbytes = recvfrom(sock, pckt->data, MAXBUFSIZE, 0, (struct sockaddr *)&remote_len, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}
			
			fp = fopen(str1, "w"); 
			if(fwrite(pckt->data, nbytes, 1, fp) <= 0){
				printf("Unable to write to file.\n");
				//exit(1);
			}

			fclose(fp);

			printf("\nNumber of bytes received is %d.\n", nbytes);
			pckt->data[nbytes] = '\0';


			}
			else{
				printf("Server does not have the file\n");
				//exit(1);
			}

			break; 

		case 1: 
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
			}


			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}

			
			if(!strcmp(buffer, "200"))
				printf("Server acknowledged receipt of the file.\n");
			else{
				printf("Did not receive server acknowledgement.\n");
				//exit(1);
			}

			str = strtok (options," ");
  			str1 = strtok (NULL, " ");

			pckt = (packet *)malloc(sizeof(packet));

			long int len = strlen(str1);
			memset(pckt->data, 0, MAXBUFSIZE);
			if(!strncmp(str1, "foo1", 4))
				fp = fopen("foo1", "r"); 
			else if(!strncmp(str1, "foo2", 4))
				fp = fopen("foo2", "r"); 
			else if(!strncmp(str1, "foo3", 4))
				fp = fopen("foo3", "r"); 
			else{
				printf("File does not exist.\n");
				exit(1);
			}		


			//determining the total file size
			fseek(fp, 0, SEEK_END);
			total_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			printf("Total size is %ld\n", total_size);
			
			if (nbytes = sendto(sock, &total_size, sizeof(total_size+1), 0,  (struct sockaddr *)&local, sizeof local) < 0){
					perror("Error in sending data from client end.\n");}
			
			pckt->index = 0;
			int sent_index = 0;
			int total_packets = (total_size)/(sizeof(pckt->data));
			printf("Total number of packets is %d\n",total_packets);
			
			total_packets++; //since truncation took place earlier	
			while(total_size){
				pckt->index++;
				sent_index = pckt->index;

				pckt->data_length = fread(pckt->data, sizeof(char), MAXBUFSIZE, fp);
			
				if ((nbytes = sendto(sock, pckt, sizeof(*pckt), 0,  (struct sockaddr *)&local, sizeof local)) < 0){
					perror("Error in sending data from client end.\n");
				}
			
				//printf("Transmitted %d bytes\n", nbytes);
				total_size = total_size - pckt->data_length;
			printf("Remaining file size: %d\n",total_size);
			}//end of while loop	
			
			fclose(fp);
		
			break; 

		case 2: 
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
			}
			
			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}
			
			if(!strncmp(buffer, "200", 3))
				printf("\nFile deleted successfully!\n");
			else if(!strncmp(buffer, "400", 3))
				printf("\nFile could not be deleted.\n");	
			else if(!strncmp(buffer, "600", 3))
				printf("File does not exist.\n");
			
			break; 
		case 3: 

			//send out the command packet ls
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
			}

			//receiving output of ls command from server
			if((nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote_len, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}
			buffer[nbytes] = '\0';
			
			printf("\nFiles present:");
			printf("\n%s\n",buffer);

			printf("Number of bytes received is %d.\n", nbytes);
			
			break;

		case 4: 
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
			}

			break; 
		default:
			printf("\nEnter option correctly.\n");

			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
			}

			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}

			
			printf("\nServer did not understand command.\n");
				//exit(1);
			

	} //end of switch case


if((sock = socket(AF_INET, SOCK_DGRAM, 0))== -1)
		perror("Socket not setup on client.\n");

	
	// building up the structure data
	memcpy((char *)&local.sin_addr, (char *)temp -> h_addr, temp->h_length);
	local.sin_family = AF_INET; //only defining for IPv4 addresses
	local.sin_port = htons(atoi(argv[2])); 	//setting port no. 
	
	printf("\n-----------------------------------");
	printf("\nPlease enter one of these options:\n");
	printf("get [file_name]\n");	
	printf("put [file_name]\n");	
	printf("delete [file_name]\n");	
	printf("ls\n");	
	printf("exit\n\n");	

	fgets(options, 20, stdin);

	} //end of while loop
	

	close(sock);
	return 0;

}



