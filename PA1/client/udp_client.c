/*****************************************************************************
​ * ​ ​ Copyright​ ​ (C)​ ​ 2017​ ​ by​ ​ Snehal Sanghvi
​ *
​ * ​ ​  Users​ ​ are  ​ permitted​ ​ to​ ​ modify​ ​ this​ ​ and​ ​ use​ ​ it​ ​ to​ ​ learn​ ​ about​ ​ the​ ​ field​ ​ of​ ​ embedded
​ * ​ ​ software.​ ​ Snehal Sanghvi​ ​ and​ ​ the​ ​ University​ ​ of​ ​ Colorado​ ​ are​ ​ not​ ​ liable​ ​ for​ ​ any​ ​ misuse​ ​ of​ ​ this​ ​ material.
​ *
*****************************************************************************/
/**
​ * ​ ​ @file​ ​ udp_client.c
​ * ​ ​ @brief​ ​ Source file having the client implementation of the udp protocol.
​​ * ​ ​ @author​ ​ Snehal Sanghvi
​ * ​ ​ @date​ ​ September ​ 23 ​ 2017
​ * ​ ​ @version​ ​ 1.0
​ *   @compiler used to process code: GCC compiler
 *	 @functionality implemented: 
 	 Created a menu-like interface for the client side having the following options:
 		 1> get <file> : requests a file from the server
 		 2> put <file> : transfers a file to the server
 		 3> delete <file> : deletes the given file from server
 		 4> ls : lists all the files present in the server directory
 		 5> exit : shuts down the server gracefully
 	 Also implemented a reliability mechanism involving timeouts and receving ACKs from the receiver.
	 Implemented a basic XOR encryption on packet data being sent and a decryption at the receiver end.
​ */

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

//setting the packet data size to 1480 bytes -> lesser than 1500 bytes (which is 1 packet chunk for ethernet)
#define MAXBUFSIZE 1480

//packet structure
typedef struct{
	int index;				//packet index
	char data[MAXBUFSIZE];	//packet data
	int data_length;		//packet data length
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
	unsigned long dataLen;
	int loop_count;
	int total_packets = 0;
	char *temp_key;
	unsigned long keyLen;
	int flag = 0;

	struct timeval timeout;

	//64 character encryption key
	char *key = "1234567890987654321234567890987654321234567890987654321234567890";

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

	//getting input from user
	fgets(options, 20, stdin);

	//comparing the user input to specific search strings
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

		//get <FILE>
		case 0:

			//sending the option to receiver eg. get foo1 
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				exit(1);
			}
			
			//function to split the string based on certain delimiters
			str = strtok (options," ");
  			str1 = strtok (NULL, " ");

  			//allocating memory to packets
			pckt = (packet *)malloc(sizeof(packet));
			pckt_ack = (packet *)malloc(sizeof(packet));

			//waiting for an 'okay' response from server
			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0)
				perror("Error in recvfrom on client.\n");

			if(!strcmp(buffer, "200"))
				printf("\nServer acknowledged request for the file.\n");

			//receiving total length of data file to expect
			if((nbytes = recvfrom(sock, &total_size, sizeof total_size, 0, (struct sockaddr *)&remote, &remote_len)) < 0)
				perror("Error in recvfrom on client.\n");

			printf("Total file length is %d\n", total_size);

			//opening the file
			fp = fopen(str1, "w");

			total_packets = (total_size)/(sizeof(pckt->data));
			printf("Total number of packets is %d\n",++total_packets);
			dataLen = sizeof(pckt->data);
			keyLen = strlen(key);

			//getting file data
			int index_temp = 1;

			//loop while you dont receive all packets
			while(total_size>0 && total_packets>0){
				loop_count = 0;
				nbytes = recvfrom(sock, pckt, packet_size, 0, (struct sockaddr *)&remote, &remote_len);

				//best case : when the received packet index is equal to local loop count
				if(pckt->index == index_temp){
					fwrite(pckt->data, sizeof(char), pckt->data_length, fp);
					total_size = total_size - pckt->data_length;
					memset(pckt, 0, packet_size);
					
					//xor encryption using 64-character key 
					while(loop_count<dataLen){
    					pckt->data[loop_count] ^= key[loop_count % (keyLen-1)]; 
    					++loop_count;
    					flag=1;
	   				}

					pckt->index = index_temp;

					//sending ack for received packet to server
					int send_bytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
					if(send_bytes)
						printf("Sent ack for packet %d.\n", index_temp);
					
					//increment expected index for next packet
					index_temp++;	
					printf("Packets left to send: %d\n", --total_packets);
				}

				//this means that ACK was not received by server. Sending only ACK
				else if(pckt->index < index_temp){
					strcpy(pckt->data, "Retransmit packet.");
					int send_bytes = sendto(sock, pckt->data, packet_size, 0, (struct sockaddr *)&local, sizeof local);
					printf("Retransmitting ACK ...\n");
					nbytes = recvfrom(sock, pckt, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
					 
					//if received index is less than expected index, ACK was lost.
					//retransmit ACK for the received packet index
					if(pckt->index == (index_temp - 1)){
						pckt->index = index_temp-1;
						int send_bytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
						if(send_bytes)
							printf("Sent ack for packet %d.\n", index_temp);
					}

				}

				memset(pckt, 0, packet_size);

			}

			if(flag)
				printf("Encrypted data has been decrypted...\n");


			fclose(fp);
			free(pckt);
			free(pckt_ack);

		break; 

		//put <FILE>
		case 1: 

			//check if server is ready to accept file
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
				//exit(1);
			}

			memset(buffer, 0, MAXBUFSIZE);

			//if server response is "200", go ahead and send the file
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
				}

				memset(buffer, 0, MAXBUFSIZE);

				if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
					perror("Error in recvfrom on client.\n");
				}
			}

			//extracting the file name using strtok
			str = strtok (options," ");
  			str1 = strtok (NULL, " ");

  			//allocating memory for packets
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
			total_packets = (total_size)/(sizeof(pckt->data));
			printf("Total number of packets is %d\n",++total_packets);
			int total_acks_received = 0;

			timeout.tv_sec = 0;
			timeout.tv_usec = 600000;	//setting a timeout of 600ms

			//setting a timeout to receive ACK from server to 600ms
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

			dataLen = sizeof(pckt->data);
			//printf("dataLen is %d\n", dataLen);
    		
			while(total_size>0 && total_packets>0){

				loop_count = 0;
				//applying some delay on the sender end to achieve better sysnchronization
				for(int k = 0; k < 5000; k++){}
				pckt->index++;
				sent_index = pckt->index;

				//encryption using the 64-character key
				while(loop_count<dataLen){
    				pckt->data[loop_count] ^= key[loop_count % (keyLen-1)]; 
    				++loop_count;
    				flag=1;
    			}

				pckt->data_length = fread(pckt->data, sizeof(char), MAXBUFSIZE, fp);
			
				//sending the encrypted packet frames to server
				nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
				printf("Sending packet id: %d.\n", pckt->index);

				int rev;

				rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
				int count = 0;
				
				//if did not receive ACK within timeout, then send again and wait for response
				while(rev < 0){
					memset(pckt_ack, 0, packet_size);
					printf("Getting error no.: %d\n", errno);
					
					//send packet again
					nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);

					printf("Retransmitted packet id: %d\n", pckt->index );
					rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
				}

				//case when receiver received correct ACK, decrease the file size to send
				if(pckt_ack->index == sent_index){
					total_size = total_size - pckt->data_length;
					printf("ACK for packet %d received.\n", sent_index);
					printf("Remaining file size: %d\n\n",total_size);
					total_acks_received++;
					printf("Packets left to send: %d\n", --total_packets);
				}

				else if(strcmp(pckt_ack->data, "Retransmit packet.")){
					for(int k=0; k< 1000; k++){}
					nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
					printf("Retransmitted packet id: %d\n", pckt->index );
					rev = recvfrom(sock, pckt_ack, packet_size, 0, (struct sockaddr *)&remote, &remote_len);
										
					while(pckt_ack->index != sent_index){
						nbytes = sendto(sock, pckt, packet_size, 0,  (struct sockaddr *)&local, sizeof local);
						printf("Retransmitted packet id: %d\n", pckt->index );
						for(int k=0; k< 20000; k++){}
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

			//modifying the socket so that the receiver goes to infinite blocking again
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;

			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));			
			
			printf("Total acks received: %d\n", total_acks_received);
			if(flag)
				printf("Encrypted data has been sent...\n");

			fclose(fp);
			free(pckt);
			free(pckt_ack);
			
			break; 

		//delete <FILE>
		case 2: 

			//send command to delete file
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
			}
			
			//waiting for command status from server
			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
			}
			
			if(!strncmp(buffer, "200", 3))
				printf("\nFile deleted successfully!\n");
			else if(!strncmp(buffer, "400", 3))
				printf("\nFile could not be deleted.\n");	
			else if(!strncmp(buffer, "600", 3))
				printf("File does not exist.\n");
			
			break; 

		//ls
		case 3: 

			//send out the command packet ls
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0){
				perror("Error in sending data from client end.\n");
			}

			//receiving output of ls command from server
			if((nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				perror("Error in recvfrom on client.\n");
			}

			buffer[nbytes] = '\0';
			
			printf("\nFiles present:");
			printf("\n%s\n",buffer);

			printf("Number of bytes received is %d.\n", nbytes);
			
			break;

		//exit
		case 4: 
			if ((nbytes = sendto(sock, options, strlen(options) - 1, 0,  (struct sockaddr *)&local, sizeof local)) < 0)
				perror("Error in sending data from client end.\n");

			break; 

		//none of the above options were entered
		default:
			printf("\nEnter option correctly.\n");

			if((nbytes = recvfrom(sock, buffer, 3, 0, (struct sockaddr *)&remote, &remote_len)) < 0)
				perror("Error in recvfrom on client.\n");
			
			if(!strcmp(buffer, "400"))
				printf("Server did not understand the given command.\n");
			else
				printf("Did not receive server acknowledgement.\n");

	} //end of switch case

	//set up the socket again for next operation
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