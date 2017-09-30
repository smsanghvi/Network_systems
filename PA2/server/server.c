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
#include <pthread.h>

#define _GNU_SOURCE		//for accept function
#define MAXLINE 1000
#define PORT 3000
#define BACKLOG 10	//for listen function call

int param;

//starting function for created thread
void *connection_handler(void *sock_thread)
{
	int sock = *(int *)sock_thread;
	char *msg;
	char client_msg[MAXLINE];
	int bytes_read;

	while((bytes_read = recv(sock, client_msg, MAXLINE, 0)) > 0){
		printf("Message received: %s\n", client_msg);
		send(sock, client_msg, strlen(client_msg), 0);
		memset(client_msg, 0, sizeof client_msg);
	}

	if(bytes_read == 0){
		printf("Client got disconnected.\n");
	}
	else if(bytes_read == -1){
		printf("Recv failed.\n");
	}

	//terminate the calling thread
	pthread_exit(&param);

	//free the resources
	free(sock_thread);

	return 0;
}	


int main (int argc, char * argv[] )
{
	int sock_listen;   //socket descriptor
	int sock_connect;  //socket descriptor
	int *sock_thread;  //socket descriptor
	struct sockaddr_in local, remote;
	char buffer[MAXLINE];
	char receive_buffer[MAXLINE];
	socklen_t remote_len;

	if(argc != 1){
		printf ("USAGE: ./server\n");
		exit(1);
	}

	//packing the local structure
	memset(&local, 0, sizeof local);
	local.sin_family = AF_INET;  			//setting IPv4 family
	local.sin_port = htons(PORT);  			//setting port number
	local.sin_addr.s_addr = INADDR_ANY;     //setting the address
	
	//calling the socket function
	if((sock_listen = socket(AF_INET, SOCK_STREAM, 0))== -1)
		perror("Socket not setup on server.\n");

	printf("Socket created succesfully on server end.\n");

	//binding the port
	if( bind(sock_listen, (struct sockaddr *)&local, sizeof(local)) == -1)
		perror("Cannot bind port on server.\n");		

	printf("Port %d successfully attached to socket.\n", PORT);

	remote_len = sizeof remote;

	//listening for new connections
	if(listen(sock_listen, BACKLOG)){
		printf("Unable to listen for connections.\n");
	}
	else{
		printf("Listening for incoming connections...\n");
	}

	while((sock_connect = accept(sock_listen, (struct sockaddr*)&remote, (socklen_t *)&remote_len))>=0){
		memset(receive_buffer, 0, MAXLINE);
		printf("Connection request accepted.\n");

		//creating new thread
		pthread_t thread;
		sock_thread = malloc(1);
		*sock_thread = sock_connect;

		if(pthread_create(&thread, NULL, connection_handler, (void *)sock_thread) <0 ){
			printf("Thread not created.\n");
			return 1;
		}
	}

	if(sock_connect < 0){
		printf("Accept failed.\n");
		return 1;
	}

	return 0;
}