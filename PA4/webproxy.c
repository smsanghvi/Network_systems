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
#include <time.h>
#include <openssl/md5.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <sys/stat.h>

#define _GNU_SOURCE		  //macro needed for accept function
#define MAXSIZE 200000
#define BACKLOG 10		  //for listen function call
#define MAXSIZE_WS  100   //for ws.conf file
#define BUF_MAX_SIZE 10000

#define SMALL	1
#define LARGE	1

#undef SMALL

extern int h_errno;
int timeout;

//obtains file length in bytes
int file_length(FILE *fp)
{
    fseek(fp, 0, SEEK_END);
    int length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return length;
}


int main (int argc, char * argv[] ){
	int sock_listen;   //socket descriptor
	int sock_connect;  //socket descriptor
	int sock_ptos;
	time_t current_time;
	struct sockaddr_in local, remote, local_proxy;
	int port_int, remote_len, bytes_read;
	char client_msg[MAXSIZE], server_resp[MAXSIZE];
	char server_resp_head[MAXSIZE], server_resp_body[MAXSIZE];
	int server_resp_head_ind, server_resp_body_ind;
	char *rqst_method;  //for splitting string
	char *rqst_url;  	//for splitting string
	char *rqst_version;  //for splitting string
	char rqst_url_copy[50];
	unsigned char temp_str[100];
	char cache[100], file_buf[100], cached_ip[100], cached_total_pair[100];
	char *result, *temp1;
	char cache_filename[100], delete_buff[100];
	pid_t pid;
	char rqst_host[100], rqst_path[100], *rqst_port_str;
	int nbytes_temp, nbytes_recv, nbytes_send, rqst_port = 80, pos=0;
	struct hostent *he;			//for gethostbyname()
	char http_request[1000];
	struct in_addr **addr_list;
	int nbytes_total = 0, filelen;
	MD5_CTX md;
	FILE *fp, *fp1;
	int fd, time_diff = 0;
	int doctype_flag=0;
	int no_bytes_write=0, no_bytes_read=0;
	struct stat stat_struct;
	char temp_cache_buf[MAXSIZE];
	char rm_file[100];

	//parse command line arguments
	if(argc != 3){
		printf ("USAGE: ./webproxy <PORT> <timeout>\n");
		exit(1);
	}

	port_int = atoi(argv[1]);
	timeout = atoi(argv[2]);


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


	sprintf(rm_file, "rm host_ip_map.txt");
	system(rm_file);
	fp1 = fopen("host_ip_map.txt", "a");
	fclose(fp1);

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

  				//computing md5sum
			  	MD5_Init (&md);
				MD5_Update (&md,rqst_url_copy, strlen(rqst_url_copy));
				MD5_Final (temp_str, &md);

				for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) 
					sprintf(&(cache[2*i]), "%02x", temp_str[i]);

				strcpy(cache_filename,  cache);
				strcat(cache_filename, ".html");
				//printf("Cache filename is %s\n", cache_filename);

				//check if the cache file already exists
				if( access( cache_filename, F_OK ) != -1 ) {
					printf("\nCache file already exists.\n");
					//check timeout
					fd = open(cache_filename, O_RDONLY);
					if(fd!=-1){
						fstat(fd, &stat_struct);
						close(fd);
					} 
					//printf("Last modification time is %ld\n", (stat_struct.st_mtime));
					current_time = time(NULL);
					//printf("Current time is %ld\n", current_time);
					time_diff = current_time - (stat_struct.st_mtime);
					//printf("Time diff is %d seconds\n", time_diff);

					if(time_diff > timeout){
						printf("Cache timed out and deleted.\n");
						//deleting the cache file
						sprintf(delete_buff, "rm %s", cache_filename);
						system(delete_buff);

		  				//only process GET requests
	  					if(!strncmp(rqst_method, "GET", 3)){
							if(strstr(rqst_url_copy, "http://")!=NULL){
								rqst_url += 7;
								strcpy(rqst_host, rqst_url);
								if(strstr(rqst_host, "/")!=NULL){
									rqst_url = strtok(rqst_url, "/");
									strcpy(rqst_host, "http://");
									strcat(rqst_host, rqst_url);
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
									strcpy(rqst_host, "https://");
									strcat(rqst_host, rqst_url);
									rqst_url = strtok(NULL, ": ");
									strcpy(rqst_path, "/");
									strcpy(rqst_path, rqst_url);
								}
								else{
									strcpy(rqst_path, "/");
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

							fp1 = fopen("host_ip_map.txt", "r");
							filelen = file_length(fp1);
							fclose(fp1);

							memset(file_buf, 0, sizeof(file_buf));
							memset(cached_ip, 0, sizeof(cached_ip));
							memset(cached_total_pair, 0, sizeof(cached_total_pair));

							//creating the hostname-ip mapping for faster access
							fp1 = fopen("host_ip_map.txt", "a+");
							fread(file_buf, sizeof(char), filelen, fp1);
							if((temp1 = strstr(file_buf, rqst_host))!=NULL){
								printf("Entry already present.\n");
								temp1 = strtok(temp1+strlen(rqst_host)+1, " \n");
								strcpy(cached_ip, temp1);
							}
							else{
								printf("Entry not present.\n");
								if ((he = gethostbyname(rqst_host)) == NULL) { 
    								herror("gethostbyname");
    								return 1;
								}
								addr_list = (struct in_addr **)he->h_addr_list;
								if(addr_list[0] == NULL){
									printf("Invalid ip addresses.\n");
									return 1;
								}
								sprintf(cached_total_pair, "%s %s\n", rqst_host, inet_ntoa(*addr_list[0]));
								fwrite(cached_total_pair, sizeof(char), strlen(cached_total_pair), fp1);
								strcpy(cached_ip, inet_ntoa(*addr_list[0]));
							}
							fclose(fp1);

							//printf("Hostname is : %s\n", he->h_name);
							//printf("inet_ntoa(*addr_list[0]) is %s\n", inet_ntoa(*addr_list[0]));


							if((sock_ptos = socket(AF_INET, SOCK_STREAM, 0))== -1)
								perror("Socket not setup on proxy.\n");

							// building up the structure data
							memset(&local_proxy, 0, sizeof local_proxy);
							local_proxy.sin_family = AF_INET; //only defining for IPv4 addresses
							local_proxy.sin_addr.s_addr = inet_addr(cached_ip);  //setting address field
							local_proxy.sin_port = htons(80); 	//setting port no. 

			 				connect(sock_ptos, (struct sockaddr *) &local_proxy, sizeof(local_proxy));
	 				
	 						//sending out the HTTP request message
		 					sprintf(http_request,"%s %s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n", rqst_method, rqst_path, rqst_host);
		 					nbytes_temp = send(sock_ptos, http_request, strlen(http_request), 0);

				 			//printf("Sent http request of %d bytes.\n", nbytes_temp);
				 			//printf("HTTP request message: %s\n", http_request);

		 					fp = fopen(cache_filename, "a");

		 					do{
			 					memset(server_resp, 0, sizeof(server_resp));
			 					memset(server_resp_head, 0, sizeof(server_resp_head));
		 						memset(server_resp_body, 0, sizeof(server_resp_body));
			 					nbytes_recv = recv(sock_ptos, server_resp, sizeof(server_resp), 0);
				 				if((result = strstr(server_resp, "<!DOCTYPE"))!=NULL){
				 					doctype_flag=1;
		 							server_resp_head_ind = result - server_resp;
			 						strncpy(server_resp_head, server_resp, server_resp_head_ind);
			 						send(sock_connect, server_resp_head, server_resp_head_ind, 0);	 	
			 						//server_resp += server_resp_head_ind;
			 						strncpy(server_resp_body, server_resp+server_resp_head_ind, nbytes_recv - server_resp_head_ind);	
					 				no_bytes_write = fwrite(server_resp_body, sizeof(char), nbytes_recv - server_resp_head_ind, fp);	
									if(!nbytes_recv<=0){
										send(sock_connect, server_resp_body, no_bytes_write, 0);
									}		 				
		 						}
		 						else if(doctype_flag==1){
									no_bytes_write = fwrite(server_resp, sizeof(char), nbytes_recv, fp);		 				
		 							if(!nbytes_recv<=0){
										send(sock_connect, server_resp, no_bytes_write, 0);
									}	
				 				}
		 					}while(nbytes_recv > 0);
		 					doctype_flag=0;

		 					fclose(fp);	 	
	 					}//end of GET block
					
					}
					else{
						//serve directly from proxy
						printf("\nServing you the cached file.\n");
		  				//only process GET requests
  						if(!strncmp(rqst_method, "GET", 3)){
							if(strstr(rqst_url_copy, "http://")!=NULL){
								rqst_url += 7;
								strcpy(rqst_host, rqst_url);
								if(strstr(rqst_host, "/")!=NULL){
									rqst_url = strtok(rqst_url, "/");
									strcpy(rqst_host, "http://");
									strcat(rqst_host, rqst_url);
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
									strcpy(rqst_host, "https://");
									strcat(rqst_host, rqst_url);
									rqst_url = strtok(NULL, ": ");
									strcpy(rqst_path, "/");
									strcpy(rqst_path, rqst_url);
								}
								else{
									strcpy(rqst_path, "/");
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

							fp = fopen(cache_filename, "r");
							filelen = file_length(fp);
							fclose(fp);

							fp = fopen(cache_filename, "r");

		 					do{
		 						memset(temp_cache_buf, 0, sizeof(temp_cache_buf));
			 					memset(server_resp, 0, sizeof(server_resp));
			 					memset(server_resp_head, 0, sizeof(server_resp_head));
		 						memset(server_resp_body, 0, sizeof(server_resp_body));
		 						nbytes_recv = fread(temp_cache_buf, sizeof(char), filelen, fp);
				 				if((result = strstr(temp_cache_buf, "<!DOCTYPE"))!=NULL){
			 						doctype_flag=1;
		 							server_resp_head_ind = result - temp_cache_buf;
			 						strncpy(server_resp_head, temp_cache_buf, server_resp_head_ind);
			 						send(sock_connect, server_resp_head, server_resp_head_ind, 0);	 	
			 						//server_resp += server_resp_head_ind;
			 						strncpy(server_resp_body, temp_cache_buf+server_resp_head_ind, nbytes_recv - server_resp_head_ind);		
									if(!nbytes_recv<=0){
										send(sock_connect, server_resp_body, nbytes_recv - server_resp_head_ind, 0);
									}		 				
		 						}
		 						else if(doctype_flag==1){	 				
		 							if(!nbytes_recv<=0){
										send(sock_connect, temp_cache_buf, nbytes_recv, 0);
									}	
				 				}
			 				}while(nbytes_recv > 0);
		 					doctype_flag=0;

		 					fclose(fp);	 
					
						}//end of GET block
					}	
				}
				else{
					printf("\nCreating a cache file.\n");

	  				//only process GET requests
  					if(!strncmp(rqst_method, "GET", 3)){
						if(strstr(rqst_url_copy, "http://")!=NULL){
							rqst_url += 7;
							strcpy(rqst_host, rqst_url);
							if(strstr(rqst_host, "/")!=NULL){
								rqst_url = strtok(rqst_url, "/");
								strcpy(rqst_host, "http://");
								strcat(rqst_host, rqst_url);
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
								strcpy(rqst_host, "https://");
								strcat(rqst_host, rqst_url);
								rqst_url = strtok(NULL, ": ");
								strcpy(rqst_path, "/");
								strcpy(rqst_path, rqst_url);
							}
							else{
								strcpy(rqst_path, "/");
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

						fp1 = fopen("host_ip_map.txt", "r");
						filelen = file_length(fp1);
						fclose(fp1);

						memset(file_buf, 0, sizeof(file_buf));
						memset(cached_ip, 0, sizeof(cached_ip));
						memset(cached_total_pair, 0, sizeof(cached_total_pair));

						//creating the hostname-ip mapping for faster access
						fp1 = fopen("host_ip_map.txt", "a+");
						fread(file_buf, sizeof(char), filelen, fp1);
						if((temp1 = strstr(file_buf, rqst_host))!=NULL){
							printf("Entry already present.\n");
							temp1 = strtok(temp1+strlen(rqst_host)+1, " \n");
							strcpy(cached_ip, temp1);
						}
						else{
							printf("Entry not present.\n");
							if ((he = gethostbyname(rqst_host)) == NULL) { 
								herror("gethostbyname");
								return 1;
							}
							addr_list = (struct in_addr **)he->h_addr_list;
							if(addr_list[0] == NULL){
								printf("Invalid ip addresses.\n");
								return 1;
							}
							sprintf(cached_total_pair, "%s %s\n", rqst_host, inet_ntoa(*addr_list[0]));
							fwrite(cached_total_pair, sizeof(char), strlen(cached_total_pair), fp1);
							strcpy(cached_ip, inet_ntoa(*addr_list[0]));
						}
						fclose(fp1);


						if((sock_ptos = socket(AF_INET, SOCK_STREAM, 0))== -1)
							perror("Socket not setup on proxy.\n");

						// building up the structure data
						memset(&local_proxy, 0, sizeof local_proxy);
						local_proxy.sin_family = AF_INET; //only defining for IPv4 addresses
						local_proxy.sin_addr.s_addr = inet_addr(inet_ntoa(*addr_list[0]));  //setting address field
						local_proxy.sin_port = htons(80); 	//setting port no. 

		 				connect(sock_ptos, (struct sockaddr *) &local_proxy, sizeof(local_proxy));
	 			
	 					//sending out the HTTP request message
		 				sprintf(http_request,"%s %s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n", rqst_method, rqst_path, rqst_host);
		 				nbytes_temp = send(sock_ptos, http_request, strlen(http_request), 0);

				 		//printf("Sent http request of %d bytes.\n", nbytes_temp);
				 		//printf("HTTP request message: %s\n", http_request);

		 				fp = fopen(cache_filename, "a");

		 				do{
			 				memset(server_resp, 0, sizeof(server_resp));
			 				memset(server_resp_head, 0, sizeof(server_resp_head));
		 					memset(server_resp_body, 0, sizeof(server_resp_body));
		 					nbytes_recv = recv(sock_ptos, server_resp, sizeof(server_resp), 0);
			 				if((result = strstr(server_resp, "<!DOCTYPE"))!=NULL){
			 					doctype_flag=1;
		 						server_resp_head_ind = result - server_resp;
			 					strncpy(server_resp_head, server_resp, server_resp_head_ind);
			 					send(sock_connect, server_resp_head, server_resp_head_ind, 0);	 	
			 					//server_resp += server_resp_head_ind;
			 					strncpy(server_resp_body, server_resp+server_resp_head_ind, nbytes_recv - server_resp_head_ind);	
				 				no_bytes_write = fwrite(server_resp_body, sizeof(char), nbytes_recv - server_resp_head_ind, fp);	
								if(!nbytes_recv<=0){
									send(sock_connect, server_resp_body, no_bytes_write, 0);
								}		 				
		 					}
		 					else if(doctype_flag==1){
								no_bytes_write = fwrite(server_resp, sizeof(char), nbytes_recv, fp);		 				
		 						if(!nbytes_recv<=0){
									send(sock_connect, server_resp, no_bytes_write, 0);
								}	
			 				}
		 				}while(nbytes_recv > 0);
		 				doctype_flag=0;

		 				fclose(fp);	 	
	 				}//end of GET block

	 			}//end of else block
	 			memset(http_request, 0, sizeof(http_request));
				memset(rqst_host, 0, sizeof(rqst_host));
				memset(rqst_path, 0, sizeof(rqst_path));
				memset(server_resp, 0, sizeof(server_resp));
				memset(server_resp_head, 0, sizeof(server_resp_head));
				memset(server_resp_body, 0, sizeof(server_resp_body));
				//memset(trunc_buf, 0, sizeof(trunc_buf));

				shutdown(sock_ptos, SHUT_RDWR);
				close(sock_ptos);

				printf("------------------------\n");

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