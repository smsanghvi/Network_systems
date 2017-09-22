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

#define MAXBUFSIZE 1480

typedef struct{
	int index;
	char data[MAXBUFSIZE];
	int data_length;
}packet; 


int main (int argc, char * argv[]){

	int sock;   //socket descriptor
	struct sockaddr_in local, remote;
	int total_size = 0;
	struct hostent *temp;
	socklen_t remote_len;
	int nbytes;
	int16_t menu_id = -1;	
	char options[20];
	char buffer[MAXBUFSIZE];
	size_t newLen;
	char *str, *str1;
	packet *pckt = NULL;
	packet *pckt_ack = NULL;
	char *temp1 = "foo1";
	FILE *fp;
	int packet_size = 0;

	struct timeval timeout;

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
	//memcpy((char *)&local.sin_addr, (char *)temp -> h_addr, temp->h_length);
	memset(&local, 0, sizeof local);
	local.sin_family = AF_INET; //only defining for IPv4 addresses
	local.sin_addr.s_addr = inet_addr(argv[1]);  //setting address field
	local.sin_port = htons(atoi(argv[2])); 	//setting port no. 

	packet_size = sizeof(pckt->index) + sizeof(pckt->data) + sizeof(pckt->data_length);

	printf("\n-------------------------------------");
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

			pckt = (packet *)malloc(sizeof(packet));

			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0)
				perror("Error in recvfrom on client.\n");
				//exit(1);

			if(!strcmp(buffer, "200"))
				printf("\nServer acknowledged request for the file.\n");

			//receiving total length of data file to expect
			if((nbytes = recvfrom(sock, &total_size, 10, 0, (struct sockaddr *)&remote, &remote_len)) < 0)
				perror("Error in recvfrom on client.\n");
				//exit(1);

			printf("Total file length is %d\n", total_size);

			fp = fopen(str1, "w");

			//getting file data
			int index_temp = 1;
			while(total_size){
				nbytes = recvfrom(sock, pckt, sizeof(*pckt), 0, (struct sockaddr *)&remote, &remote_len);
				printf("Got here\n");
				printf("nbytes received are %d\n", nbytes);

				if(pckt->index == index_temp){
					printf("pckt->index is %d\n", pckt->index);
					printf("pckt->data_length is %d\n", pckt->data_length);
					fwrite(pckt->data, sizeof(char), pckt->data_length, fp);
					total_size = total_size - pckt->data_length;
					printf("Total size is %d\n", total_size);
					memset(pckt, 0, sizeof(*pckt));
					pckt->index = index_temp;
					index_temp++;	
					printf("Remaining file length is %d\n", total_size);
				}

			}

			printf("Hi there\n");

			fclose(fp);
			free(pckt);

			break; 

		case 1: 
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
			}

			memset(buffer, 0, MAXBUFSIZE);

			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}

			
			if(!strcmp(buffer, "200"))
				printf("Server acknowledged receipt of the file.\n");
			else{
				printf("Did not receive server acknowledgement.\n");
				
				if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
				}

				memset(buffer, 0, MAXBUFSIZE);

				if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
					perror("Error in recvfrom on client.\n");
				//exit(1);
				}
				//exit(1);
			}

			str = strtok (options," ");
  			str1 = strtok (NULL, " ");

			pckt = (packet *)malloc(sizeof(packet));
			pckt_ack = (packet *)malloc(sizeof(packet));

			long int len = strlen(str1);
			memset(buffer, 0, MAXBUFSIZE);
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

			if(fp == NULL){
				printf("File does not exist.\n");
				exit(1);
			}			

			//determining the total file size
			fseek(fp, 0, SEEK_END);
			int total_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			printf("Total size is %d\n", total_size);

			//sending the total file size to expect
			if ((nbytes = sendto(sock, &total_size, sizeof(total_size)+1, 0,  (struct sockaddr *)&local, sizeof local)) < 0)
				perror("Error in sending data from client end.\n");

			pckt->index = 0;
			int sent_index = 0;
			int total_packets = (total_size)/(sizeof(pckt->data));
			printf("Total number of packets is %d\n",++total_packets);
			int total_acks_received = 0;

			timeout.tv_sec = 0;
			timeout.tv_usec = 600000;	//setting a timeout of 600ms

			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

			while(total_size>0 && total_packets>0){

				//applying some delay on the sender end to achieve better sysnchronization
				for(int k = 0; k < 30000; k++){}
				pckt->index++;
				sent_index = pckt->index;

				pckt->data_length = fread(pckt->data, sizeof(char), MAXBUFSIZE, fp);
			
				nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);

				printf("Sending packet id: %d.\n", pckt->index);

				int recvd_packet_id, rev;

				rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
				int count = 0;
				
				while(rev < 0){
					memset(pckt_ack, 0, packet_size);
					printf("Getting error no.: %d\n", errno);
					
					//send packet again
					nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);

					printf("Retransmitted packet id: %d\n", pckt->index );
					rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
				}

				//case when receiver received correct ACK
				if(pckt_ack->index == sent_index){
					total_size = total_size - pckt->data_length;
					printf("ACK for packet %d received.\n", sent_index);
					printf("Remaining file size: %d\n\n",total_size);
					total_acks_received++;
					printf("Packets left to send: %d\n", --total_packets);
				}

				//case when receiver timed out and didn't receive ACK
				else if(strcmp(pckt_ack->data, "Retransmit packet.")){
					for(int k=0; k< 1000; k++){}
					nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
					printf("Retransmitted packet id: %d\n", pckt->index );
					rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
					
					/*while(rev < 0){
						memset(pckt_ack, 0, packet_size);
						printf("Getting error no.: %d\n", errno);
						//send packet again
						nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
						printf("Retransmitted packet id: %d\n", pckt->index );
						rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
					}

					if(pckt_ack->index == sent_index){
						total_size = total_size - pckt->data_length;
						printf("ACK for packet %d received.\n", sent_index);
						printf("Remaining file size: %d\n\n",total_size);
						total_acks_received++;
					}

					//case when receiver timed out and didn't receive ACK
					else if(strcmp(pckt_ack->data, "Retransmit packet.")){
						for(int k=0; k< 1000; k++){}
						nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
						printf("Retransmitted packet id: %d\n", pckt->index );
						rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
					}*/

					
					while(pckt_ack->index != sent_index){
						for(int k=0; k< 30000; k++){}
						nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
						printf("Retransmitted packet id: %d\n", pckt->index );
						rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
					}

					total_acks_received++;
					printf("Packets left to send: %d\n", --total_packets);
				}

				//ack lost
				else{
					printf("ACK lost\n");
					break;
				}

			}	
			
			printf("Total acks received: %d\n", total_acks_received);

			fclose(fp);
			free(pckt);
			free(pckt_ack);
			
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
			if((nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
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
			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
				//exit(1);
			}

			
			if(!strcmp(buffer, "400"))
				printf("Server did not understand the given command.\n");
			else{
				printf("Did not receive server acknowledgement.\n");
				//exit(1);
			}

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

	memset(options, 0, 20);
	fgets(options, 20, stdin);

	} //end of while loop
	

	close(sock);
	return 0;

}
