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

#define _GNU_SOURCE		  //macro needed for accept function
#define MAXSIZE 100000
#define BACKLOG 10		  //for listen function call
#define MAXSIZE_WS  100   //for ws.conf file
#define BUF_MAX_SIZE 10000

extern int h_errno;

int main (int argc, char * argv[] ){
	int sock_listen;   //socket descriptor
	int sock_connect;  //socket descriptor
	int sock_ptos;
	struct sockaddr_in local, remote, local_proxy;
	int port_int, remote_len, bytes_read;
	char client_msg[MAXSIZE], server_resp[MAXSIZE];
	char *rqst_method;  //for splitting string
	char *rqst_url;  	//for splitting string
	char *rqst_version;  //for splitting string
	char rqst_url_copy[20];
	pid_t pid;
	char rqst_host[100], rqst_path[100], *rqst_port_str;
	int nbytes_temp, nbytes_recv, nbytes_send, rqst_port = 80;
	struct hostent *he;			//for gethostbyname()
	char http_request[1000];
	struct in_addr **addr_list;
	int nbytes_total = 0;

	//parse command line arguments
	if(argc != 2){
		printf ("USAGE: ./webproxy <PORT>\n");
		exit(1);
	}

	port_int = atoi(argv[1]);
	
	//checking if port no. is greater than 1024
	if(port_int < 1024){
		printf("Enter port no. greater than 1024.\n");
		exit(1);
	}

	//packing the local structure
	memset(&local, 0, sizeof local);
	local.sin_family = AF_INET;  			//setting IPv4 family
	local.sin_port = htons(port_int);  		//setting port number from config file
	local.sin_addr.s_addr = INADDR_ANY;     //setting the address


	//calling the socket function
	if((sock_listen = socket(AF_INET, SOCK_STREAM, 0))== -1)
		perror("Socket not setup on proxy.\n");

	printf("Socket created succesfully on proxy end.\n");

	
	int temp = 1;
	
	//function to be able to reuse socket numbers
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, (const void *)&temp , sizeof(int)); 

	//binding the port
	if( bind(sock_listen, (struct sockaddr *)&local, sizeof(local)) == -1)
		perror("Cannot bind port on proxy.\n");		

	printf("Port %d successfully attached to proxy.\n", port_int);

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
		//setsockopt(sock_connect, SOL_SOCKET, SO_REUSEADDR, (const void *)&temp , sizeof(int)); 
		sock_connect = accept(sock_listen, (struct sockaddr*)&remote, (socklen_t *)&remote_len);
		if(sock_connect<0){
			perror("Cannot accept incoming connections.\n");
			exit(1);
		}	

		printf("\n\nAccepted a connection from %s\n", inet_ntoa(remote.sin_addr));

		if( (pid = fork()) == 0){
			printf("Process id is %d\n", getpid());
			memset(client_msg, 0, sizeof(client_msg));

			bytes_read = recv(sock_connect, client_msg, MAXSIZE, 0);
			
			//close(sock_listen);
			if(bytes_read > 0){			
				printf("--------------------\n");
				//printf("Bytes read: %d\n", bytes_read);
				printf("Message received is: %s\n", client_msg);

				//parsing
				rqst_method = strtok (client_msg," ");
				printf("Request method: %s\n", rqst_method);
  				rqst_url = strtok (NULL, " ");
  				printf("Request URL: %s\n", rqst_url);
  				strcpy(rqst_url_copy, rqst_url);
  				rqst_version = strtok (NULL, " \n");
  				printf("Request version: %s\n", rqst_version);


					if(strstr(rqst_url_copy, "http://")!=NULL){
						rqst_url += 7;
						strcpy(rqst_host, rqst_url);
						if(strstr(rqst_host, "/")!=NULL){
							rqst_url = strtok(rqst_url, "/");
							strcpy(rqst_host, rqst_url);
							rqst_url = strtok(NULL, ": ");
							strcpy(rqst_path, "/");
							strcat(rqst_path, rqst_url);
						}
						else{
							strcpy(rqst_path, "/");
						}
					}
					else if(strstr(rqst_url_copy, "https://")!=NULL){
						rqst_url += 8;
						strcpy(rqst_host, rqst_url);
						if(strstr(rqst_host, "/")!=NULL){
							rqst_url = strtok(rqst_url, "/");
							strcpy(rqst_host, rqst_url);
							rqst_url = strtok(NULL, ": ");
							strcpy(rqst_path, rqst_url);
						}
						else{
							strcpy(rqst_path, "");
						}
					}
					else{
						if(strstr(rqst_url_copy, ":")==NULL){
							strcpy(rqst_host, rqst_url);
							rqst_port = 80;
						}
						else{
							rqst_url = strtok(rqst_url, ":");
							strcpy(rqst_host, rqst_url);
							rqst_port_str = strtok(NULL, " ");
							rqst_port = atoi(rqst_port_str);
						}					
					}

				printf("Rqst host is %s\n", rqst_host);
				printf("Rqst path is %s\n", rqst_path);


				if ((he = gethostbyname(rqst_host)) == NULL) { 
    				herror("gethostbyname");
    				return 1;
				}


				addr_list = (struct in_addr **)he->h_addr_list;
			
				if(addr_list[0] == NULL){
					printf("Invalid ip addresses.\n");
					return 1;
				}
				//printf("Hostname is : %s\n", he->h_name);


				if((sock_ptos = socket(AF_INET, SOCK_STREAM, 0))== -1)
					perror("Socket not setup on proxy.\n");

				// building up the structure data
				memset(&local_proxy, 0, sizeof local_proxy);
				local_proxy.sin_family = AF_INET; //only defining for IPv4 addresses
				local_proxy.sin_addr.s_addr = inet_addr(inet_ntoa(*addr_list[0]));  //setting address field
				local_proxy.sin_port = htons(80); 	//setting port no. 

	 			connect(sock_ptos, (struct sockaddr *) &local_proxy, sizeof(local_proxy));
	 		
	 			//sending out the HTTP request message
	 			//sprintf(http_request,"%s %s %s\r\nHost: %s:80\r\nConnection: Close\r\n\r\n", rqst_method, rqst_path, rqst_version, rqst_host);
	 			sprintf(http_request,"GET %s\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n", rqst_path, rqst_host);
	 			nbytes_temp = send(sock_ptos, http_request, strlen(http_request), 0);

		 		printf("Sent http request of %d bytes.\n", nbytes_temp);
		 		printf("HTTP request message: %s\n", http_request);

		 		do{
		 			memset(server_resp, 0, sizeof(server_resp));
		 			nbytes_recv = recv(sock_ptos, server_resp, sizeof(server_resp), 0);
					printf("nbytes_recv is %d\n", nbytes_recv);
					if(!nbytes_recv<=0){
						send(sock_connect, server_resp, nbytes_recv, 0);
					}
		 		}while(nbytes_recv > 0);

		 		printf("Hey there!\n");

				//printf("Received data is -\n--------------------------------\n%s\n", server_resp);
	 			memset(http_request, 0, sizeof(http_request));
				memset(rqst_host, 0, sizeof(rqst_host));
				memset(rqst_path, 0, sizeof(rqst_path));
				memset(server_resp, 0, sizeof(server_resp));

				shutdown(sock_ptos, SHUT_RDWR);
				close(sock_ptos);

			}	

			shutdown(sock_connect, SHUT_RDWR);
			close(sock_connect);
		}
		else if (pid == -1){
			printf("Could not fork. Exiting...\n");
			exit(1);
		}

		//in parent
		else{
			//close the connect socket parent only needs listen socket
			close(sock_connect);
		}
	}

	printf("Closing and exiting...\n");
	close(sock_listen);


	return 0;
}