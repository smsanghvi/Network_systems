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

#define _GNU_SOURCE		//for accept function
#define MAXSIZE 1024
#define BACKLOG 10	//for listen function call
#define MAXSIZE_WS  100   //for ws.conf file
#define BUF_MAX_SIZE 10000

int param;
FILE *fp;
struct stat statistics;
FILE *fp;
volatile sig_atomic_t flag = 1;
volatile int flag1 = 1;

/* The signal handler just clears the flag and re-enables itself. */
void catch_alarm (int sig)
{
  flag = 0;
  printf("Time out.\n");
  signal (sig, catch_alarm);
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
	char client_msg_post[MAXSIZE];
	int bytes_read;
	char *rqst_method;  //for splitting string
	char *rqst_url;  	//for splitting string
	char *rqst_version;  //for splitting string
	char rqst_url_copy[20];
	char *str;
	char root_path[100];
	char total_path[150];
	char timeout_str[10];
	char content[50][50];
	int port_int, timeout;
	char buf_ws[MAXSIZE_WS];
	int count = 0;
	char index0[10];
	char index1[10];
	char index2[10];
	FILE *fp1;
	int pos = 0;
	char eg[1000];
	int file_presence;
	int no_of_content_types = 0;  //for ws.conf content types
	char extension[10];
	char *extension_temp;
	char content_type_out[20];
	uint32_t file_length;
	uint32_t file_bytes;
	char buffer_data[BUF_MAX_SIZE];
	char *post_buffer_data;
	char *substring;


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


	while(!feof(fp)){
		//reading a line at a time
		fgets(buf_ws, MAXSIZE_WS, fp);
		str = strtok(buf_ws, "\n");

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
			strcpy(total_path, str);
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
			//handling the keep-alive timeout value
			fgets(buf_ws, MAXSIZE_WS, fp);
			strtok(buf_ws, " ");
			strtok(NULL, " ");
			str = strtok(NULL, " ");
			strcpy(timeout_str, str);
			timeout = atoi(timeout_str);
			break;
		}
		else{
			count=0;
		}

	}
	

	//packing the local structure
	memset(&local, 0, sizeof local);
	local.sin_family = AF_INET;  			//setting IPv4 family
	local.sin_port = htons(port_int);  		//setting port number from config file
	local.sin_addr.s_addr = INADDR_ANY;     //setting the address
	

	//logging the parsing output
	printf("Parsing the server configuration file...\n");
	printf("Port no. is %d\n", port_int);
	printf("Root path is %s\n", root_path);
	printf("Default pages are %s, %s, %s\n", index0, index1, index2);
	printf("Content types are:\n");
	for(int n = 0; n < 2*no_of_content_types; n=n+2){
		printf("%s %s", content[n], content[n+1]);
	}
	printf("Timeout value is %d\n", timeout);
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

	printf("Port %d successfully attached to socket.\n", port_int);

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
		setsockopt(sock_connect, SOL_SOCKET, SO_REUSEADDR, (const void *)&temp , sizeof(int)); 

		sock_connect = accept(sock_listen, (struct sockaddr*)&remote, (socklen_t *)&remote_len);
		if(sock_connect<0){
			perror("Cannot accept incoming connections.\n");
			exit(1);
		}	

		printf("\n\nAccepted a connection from %s\n", inet_ntoa(remote.sin_addr));

		
		if( (pid = fork()) == 0){
			printf("Process id is %d\n", getpid());
			//close(sock_listen);
			while(((bytes_read = recv(sock_connect, client_msg, MAXSIZE, 0)) > 0) && flag){
				//printf("Message received: %s\n", client_msg);
				//puts(client_msg);
				
				if(strstr(client_msg, "Connection: keep-alive") != NULL){
	  				//setting alarm timeout value
  					signal(SIGALRM, catch_alarm);
  					alarm(timeout);
  					flag1 = 0;
  					printf("Request to keep connection alive.\n");
  					printf("Timeout set for %d seconds.\n", timeout);
				}
				else{
					flag1 = 1;
				}
				
				strcpy(client_msg_post, client_msg);

				rqst_method = strtok (client_msg," ");
				printf("Request method: %s\n", rqst_method);
  				rqst_url = strtok (NULL, " ");
  				printf("Request URL: %s\n", rqst_url);
  				strcpy(rqst_url_copy, rqst_url);
  				rqst_version = strtok (NULL, " \n");
  				printf("Request version: %s\n", rqst_version);
  				//strcpy(modified_path, rqst_url);
  				strcat(total_path, rqst_url);
  				printf("File path : ");
  				puts(total_path); 


  				//special case - when url is just '/' with strlen=1 -> return default index.html
  				if((strlen(rqst_url)==1) && (!strcmp(rqst_method, "GET"))){
  					strcpy(rqst_url, index0); 
  					strcat(total_path, rqst_url);
   					fp = fopen(total_path, "r");
  					stat(total_path, &statistics);
  					file_length = statistics.st_size;
  					printf("File size is %d\n", file_length);
  					if(!flag1){
	  					sprintf(eg,"HTTP/1.1 200 OK\r\nContent-Type: %s; charset=UTF-8\r\nContent-Length: %d\r\nConnection: Keep-alive\r\n\r\n", content_type_out, file_length);
  						printf("Connection: keep alive detected\n");
  					}
					else{
						sprintf(eg,"HTTP/1.1 200 OK\r\nContent-Type: %s; charset=UTF-8\r\nContent-Length: %d\r\nConnection: Close\r\n\r\n", content_type_out, file_length);						
						printf("Connection: keep alive not detected\n");
					}  					
					send(sock_connect, eg, strlen(eg), 0);
					while((file_bytes = fread(buffer_data, sizeof(char), BUF_MAX_SIZE, fp)) > 0){
						send(sock_connect, buffer_data, file_bytes, 0);
					}
					fclose(fp);
					//shutdown(sock_connect, 1);
					memset(client_msg, 0, sizeof client_msg);
					break;  					
  				}


  				//get extension
  				strtok(rqst_url_copy, ".");
  				extension_temp = strtok(NULL, " ");
  				strcpy(extension, ".");
  				strcat(extension, extension_temp);
  				//printf("Extension is %s\n", extension);

  				//mapping - code to map the file extension to file content type 
  				int count_temp=0;
  				int flag=0;

  				for(count_temp=0; count_temp<no_of_content_types; count_temp++){
  					if(!strcmp(extension, content[count_temp])){
  						strcpy(content_type_out, content[count_temp+1]);
  						flag=1;
  						break;
  					}
  				}

  				if(flag){
  					printf("Content-Type is %s\n", content_type_out);
  				}
  				else{
  					printf("Did not find content-type mapping.\n");
  				}

  				//checking if file is present in the path or not
  				file_presence = access (total_path, F_OK);

  				//for GET method
  				if(file_presence && (!strcmp(rqst_method, "GET"))){
  					//file not present - throw 404
  					sprintf(eg,"%s 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<!DOCTYPE html>\r\n \
  					<html><head><title>Simple webserver</title></head>\r\n<body> \
  					<p><b>404: File not present</b></p>\r\n \
  					</body></html>\r\n", rqst_version);
					send(sock_connect, eg, strlen(eg), 0);
					//shutdown(sock_connect, 1);
					memset(client_msg, 0, sizeof client_msg);  		
					exit(1);			
  				}
  				//for GET method
  				else if(!file_presence && (!strcmp(rqst_method, "GET"))){
  					//if return is 0, file is present
  					fp = fopen(total_path, "r");
  					stat(total_path, &statistics);
  					file_length = statistics.st_size;
  					if(!flag1){
	  					sprintf(eg,"%s 200 OK\r\nContent-Type: %s; charset=UTF-8\r\nContent-Length: %d\r\nConnection: Keep-alive\r\n\r\n", rqst_version, content_type_out, file_length);
  						printf("Connection: keep alive detected\n");
  					}
					else{
						sprintf(eg,"%s 200 OK\r\nContent-Type: %s; charset=UTF-8\r\nContent-Length: %d\r\nConnection: Close\r\n\r\n", rqst_version, content_type_out, file_length);						
						printf("Connection: keep alive not detected\n");
					}
					send(sock_connect, eg, strlen(eg), 0);
					while((file_bytes = fread(buffer_data, sizeof(char), BUF_MAX_SIZE, fp)) > 0){
						send(sock_connect, buffer_data, file_bytes, 0);
					}
					fclose(fp);
					//shutdown(sock_connect, 1);
					memset(client_msg, 0, sizeof client_msg);
  				}
  				//for POST method, if extension is .html
  				else if(!file_presence && (!strcmp(rqst_method, "POST")) && (!strcmp(extension, ".html"))){
  					//extracting data that you want to append
  					post_buffer_data = strstr(client_msg_post, "<html>");
  					printf("data: %s\n", post_buffer_data);
  					int length_data = strlen(post_buffer_data);
  					fp = fopen(total_path, "r");
  					stat(total_path, &statistics);
  					file_length = statistics.st_size;
  					sprintf(eg,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", 
  						 content_type_out, file_length+length_data);
					send(sock_connect, eg, strlen(eg), 0);
					send(sock_connect, post_buffer_data, strlen(post_buffer_data), 0);
					while((file_bytes = fread(buffer_data, sizeof(char), BUF_MAX_SIZE, fp)) > 0){
						send(sock_connect, buffer_data, file_bytes, 0);
					}
					fclose(fp);
					//shutdown(sock_connect, 1);
					memset(client_msg, 0, sizeof client_msg);
  				}
  				//for POST method, if extension is not .html
  				else if(!strcmp(rqst_method, "POST") && strcmp(extension, ".html")){
  					sprintf(eg,"%s 501 Not Implemented\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<!DOCTYPE html>\r\n<html><body>501 Not implemented : file extension problem</body></html>\r\n", \
  						rqst_version);
					send(sock_connect, eg, strlen(eg), 0);
					shutdown(sock_connect, 1);
					memset(client_msg, 0, sizeof client_msg);
  				}


  				//the next 3 statements are a workaround around the previous value being retained problem
  				memset(rqst_url, 0, sizeof rqst_url);
  				memset(total_path, 0, sizeof total_path);
  				strcpy(total_path, root_path);

			}

			flag = 1;
			flag1 = 1; 		
			printf("Out of loop...\n");

			close(sock_connect);
			exit(0);
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
	//close(sock_connect);
	return 0;
}