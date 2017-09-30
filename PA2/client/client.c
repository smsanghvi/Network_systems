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

#define MAXLINE 1000
#define PORT 3000

int main(int argc, char **argv){
	int sock; //socket descriptor 
	struct sockaddr_in local,remote;
	socklen_t local_len, remote_len;
	char buffer[MAXLINE];
	char receive_buffer[MAXLINE];
	int bytes_sent;
	int bytes_received;
	struct hostent *temp;

	local_len = sizeof(local);

	if (argc != 2)
	{
		printf("USAGE: <server_ip>\n");
		exit(1);
	}

	temp = gethostbyname(argv[1]);
	if(!temp)
		perror("No such host.");

	//calling the socket function
	if((sock = socket(AF_INET, SOCK_STREAM, 0))== -1){
		perror("Socket not setup on client.\n");
		exit(1);
	}

	printf("Socket set up on client.\n");

	//populating the structure data
	memset(&local, 0, sizeof local);

	local.sin_family = AF_INET; //only defining for IPv4 addresses
	local.sin_addr.s_addr = inet_addr(argv[1]);  //setting address field
	local.sin_port = htons(PORT); 	//setting port no.


	//connect to a server
	if(connect(sock, (struct sockaddr*)&local, local_len) !=0){
		printf("Cannot connect to server.\n");
		exit(1);
	}

	printf("Connection to server succeeded.\n");
	printf("Message to be sent:\n");

	//start sending out data to server
	while(fgets(buffer, MAXLINE, stdin) != NULL){
		memset(receive_buffer, 0, sizeof receive_buffer);
		bytes_sent = send(sock, buffer, strlen(buffer)-1, 0);
		if(bytes_sent==-1){
			printf("Sending failed.\n");
		}
		else{
			printf("%d bytes sent.\n", bytes_sent);
		}

		//receive ack from server
		bytes_received = recv(sock, receive_buffer, MAXLINE, 0);
		if(bytes_received<0)
			printf("Recv failed.\n");
		else{
			printf("Received %d bytes\n", bytes_received);
			printf("%s\n", receive_buffer );
		}			
		memset(buffer, 0, sizeof receive_buffer);
	}

	close(sock);

	return 0;
}