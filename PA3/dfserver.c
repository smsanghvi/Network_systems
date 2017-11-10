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
#define MAXSIZE 1000
#define BACKLOG 10

struct sockaddr_in local, remote;
int sock_listen;   //socket descriptor
int sock_connect;  //socket descriptor
char folder[20];
int port_no;


int main(int argc, char **argv){

	socklen_t remote_len;
	pid_t pid;
	int bytes_read;
	int temp = 1;
	char client_msg[MAXSIZE];
	char local_username[20];
	char local_password[20];
	char recv_username[20];
	char recv_password[20];

	//parse command line arguments
	if(argc != 3){
		printf ("USAGE: dfserver <DFS folder name> <port no>\n");
		exit(1);
	}

	strncpy(folder, argv[1], 20);
	port_no = atoi(argv[2]);

	//snipping off the first character if it is /
	if(folder[0] == '/'){
		memmove(folder, folder+1, strlen(folder));
	}

	//packing the local structure
	memset(&local, 0, sizeof local);
	local.sin_family = AF_INET;  			//setting IPv4 family
	local.sin_port = htons(port_no);  		//setting port number from config file
	local.sin_addr.s_addr = INADDR_ANY;     //setting the address


	//calling the socket function
	if((sock_listen = socket(AF_INET, SOCK_STREAM, 0))== -1)
		perror("Socket not setup on server.\n");

	printf("Socket created succesfully on server end.\n");


	//binding the port
	if( bind(sock_listen, (struct sockaddr *)&local, sizeof(local)) == -1)
		perror("Cannot bind port on server.\n");		

	printf("Port %d successfully attached to socket.\n", port_no);
 
	remote_len = sizeof remote;

	//listening for new connections
	if(listen(sock_listen, BACKLOG)){
		printf("Unable to listen for connections.\n");
	}
	else{
		printf("Listening for incoming connections...");
	}

	printf("\n");

	while(1){
		//accept a connection
		setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, (const void *)&temp , sizeof(int)); 

		sock_connect = accept(sock_listen, (struct sockaddr*)&remote, (socklen_t *)&remote_len);
		if(sock_connect<0){
			perror("Cannot accept incoming connections.\n");
			exit(1);
		}	

		printf("\n\nAccepted a connection from %s\n", inet_ntoa(remote.sin_addr));
		char temp[10];
		if( (pid = fork()) == 0){
			printf("Process id is %d\n", getpid());
			//close(sock_listen);
			memset(recv_username, 0, strlen(recv_username));
			memset(temp, 0, strlen(temp));
			memset(recv_password, 0, strlen(recv_password));
			if((bytes_read = recv(sock_connect, recv_username, MAXSIZE, 0)) > 0){}
				printf("Received username is %s\n", recv_username);
			recv(sock_connect, temp, 10, 0);
			if((bytes_read = recv(sock_connect, recv_password, MAXSIZE, 0)) > 0){}
				printf("Received password is %s\n", recv_password);			
		}
	}

	return 0;
}