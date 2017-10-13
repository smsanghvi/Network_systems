#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
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

#define _GNU_SOURCE		//for accept function
#define MAXSIZE 1024
#define PORT 8888
#define BACKLOG 10	//for listen function call
#define MAXSIZE_WS  100   //for ws.conf file

int param;
FILE *fp;

void parse_config_file(){

}

int main (int argc, char * argv[] )
{
	int sock_listen;   //socket descriptor
	int sock_connect;  //socket descriptor
	int *sock_thread;  //socket descriptor
	pid_t pid;
	struct sockaddr_in local, remote;
	char buffer[MAXSIZE];
	char receive_buffer[MAXSIZE];
	socklen_t remote_len;
	char client_msg[MAXSIZE];
	int bytes_read;
	char *rqst_method;  //for splitting string
	char *rqst_url;  	//for splitting string
	char *rqst_version;  //for splitting string
	char *str;
	char root_path[100];
	char content[50][50];
	int port_int;
	char buf_ws[MAXSIZE_WS];
	int count = 0;
	char index0[10];
	char index1[10];
	char index2[10];
	FILE *fp1;
	int pos = 0;
	char eg[1000];
	int no_of_content_types = 0;


	if(argc != 1){
		printf ("USAGE: ./server\n");
		exit(1);
	}

	//parse the ws.conf file and extract various parameters
	if ((fp = fopen("ws.conf","r")) == NULL)		//check if ws.conf file exists
	{
		printf("Config file ws.conf not found.\nExiting...\n");
		exit(1);
	}

	char path_temp[200] = "/home/ssanghvi/Network_systems/PA2/server";
	char modified_path[100];
	//strcat(path_temp, modified_path);

	/*
	fp1 = fopen(path_temp, "r");
	if(fp1==NULL){
		perror("Could not open file.\n");
		exit(1);
	}
	char tetete[100];
	printf("Opened\n");
	fread(tetete, 1, 100, fp1);
	printf("tetete is %s\n", tetete);
	fclose(fp1);*/

	while(!feof(fp)){
		//reading a line at a time
		fgets(buf_ws, MAXSIZE_WS, fp);
		str = strtok(buf_ws, "\n");
		//printf("str is %s\n", str);

		//extracting port number and storing it in port_int
		if(count==1){
			strtok(buf_ws, " ");
			str = strtok(NULL, " ");
			port_int = atoi(str);

			//exit if the chosen port number is less than 1024
			if(port_int < 1024){
				printf("Port no. is less than 1024.\n");
				exit(1);
			}
		}

		//extracting document root and storing it in root_path
		if(count==2){
			strtok(buf_ws, "\"");
			str = strtok(NULL, "\"");
			strcpy(root_path, str);
		}

		//extracting directory indices which serve as default web pages
		if(count==3){
			strtok(buf_ws, " ");
			str = strtok(NULL, " ");
			strcpy(index0,str);
			str = strtok(NULL, " ");
			strcpy(index1,str);
			str = strtok(NULL, " ");
			strcpy(index2,str);
		}


		if(count==4){
			//printf("%c\n", buf_ws[0]);
			//while(strcmp(buf_ws, "#connection timeout")){
			while(buf_ws[0]!='#'){
				str = strtok(buf_ws, " ");
				strcpy(content[pos], str);
				pos++;
				str = strtok(NULL, "\"");
				strcpy(content[pos], str);
				pos++;
				fgets(buf_ws, MAXSIZE_WS, fp);
				no_of_content_types++;
			}
			count=5;
		}


		if(!strcmp(str, "#serviceport number")){
			count=1;
		}
		else if(!strcmp(str, "#document root")){
			count=2;
		}
		else if(!strcmp(str, "#default web page")){
			count=3;
		}
		else if(!strcmp(str, "#Content-Type which the server handles")){
			count=4;
		}
		else if(count==5){
			//printf("In connection timeout\n");
		}
		else{
			count=0;
		}

		//printf("Count:%d\n", count);

	}
	

	//packing the local structure
	memset(&local, 0, sizeof local);
	local.sin_family = AF_INET;  			//setting IPv4 family
	local.sin_port = htons(port_int);  		//setting port number from config file
	local.sin_addr.s_addr = INADDR_ANY;     //setting the address
	
	printf("Parsing the server configuration file...\n");
	printf("Port no. is %d\n", port_int);
	printf("Root path is %s\n", root_path);
	printf("Default pages are %s, %s, %s\n", index0, index1, index2);
	printf("Content types are:\n");
	for(int n = 0; n < 2*no_of_content_types; n=n+2){
		printf("%s %s", content[n], content[n+1]);
	}
	printf("Finished parsing.\n\n");

	//calling the socket function
	if((sock_listen = socket(AF_INET, SOCK_STREAM, 0))== -1)
		perror("Socket not setup on server.\n");

	printf("Socket created succesfully on server end.\n");

	int temp = 1;
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, (const void *)&temp , sizeof(int)); 

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

	while(1){
		//accept a connection
		setsockopt(sock_connect, SOL_SOCKET, SO_REUSEADDR, (const void *)&temp , sizeof(int)); 

		sock_connect = accept(sock_listen, (struct sockaddr*)&remote, (socklen_t *)&remote_len);
		if(sock_connect<0){
			perror("Cannot accept incoming connections.\n");
			exit(1);
		}	

		printf("Accepted a connection from %s\n", inet_ntoa(remote.sin_addr));
		
		if( (pid = fork()) == 0){

			//close(sock_listen);

			while((bytes_read = recv(sock_connect, client_msg, MAXSIZE, 0)) > 0){
				//printf("Message received: %s\n", client_msg);
				puts(client_msg);
				//printf("Hooray!\n");
				rqst_method = strtok (client_msg," ");
				printf("Request method: %s\n", rqst_method);
  				rqst_url = strtok (NULL, " /");
  				printf("Request URL: %s\n", rqst_url);
  				rqst_version = strtok (NULL, " \n");
  				printf("Request version: %s\n", rqst_version);
  				strcpy(modified_path, rqst_url);
  				strcat(path_temp, modified_path);
  				printf("Here\n");
  				/*const char webpage[] = 
  				"HTTP/1.1 200 OK\r\n"
  				"Content-Type: text/html; charset=UTF-8\r\n\r\n"
  				"<!DOCTYPE html>\r\n"
  				"<html><head><title>Webserver header</title></head>\r\n"
  				"<body>Hello world!</body></html>\r\n";*/
  				sprintf(eg,"HTTP/1.1 200 OK\r\nContent-Type: %s; charset=UTF-8\r\n\r\n<!DOCTYPE html>\r\n \
  					<html><head><title>Simple webserver</title></head>\r\n<body> \
  					<p>A paragraph</p>\r\n<p><img src=\"www/images/exam.gif\">exam.gif</a> \
  					</body></html>\r\n", "text/html");
				send(sock_connect, eg, strlen(eg), 0);
				shutdown(sock_connect, 1);
				printf("Data sent.\n");
				memset(client_msg, 0, sizeof client_msg);
				//close(sock_connect);
			}

			/*if(bytes_read == 0){
				printf("Client got disconnected.\n");
			}
	
			else if(bytes_read == -1){
				printf("Recv failed.\n");
			}*/

			close(sock_connect);
		}

		else if (pid == -1){
			printf("Could not fork. Exitting...\n");
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
	//close(sock_connect);
	return 0;
}