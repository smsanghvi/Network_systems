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
#include <dirent.h>			//checking for directories
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define MAX_CONF_SIZE 1000
#define NO_OF_CONNECTIONS 4
#define MAXSIZE 1000000
#define BACKLOG 10

struct sockaddr_in local, remote;
int sock_listen;   //socket descriptor
int sock_connect;  //socket descriptor
char folder[20];
int port_no;


int main(int argc, char **argv){

	socklen_t remote_len;
	pid_t pid;
	int bytes_read, no, temp = 1;
	char *temp_str;
	int recv_authenticate_length;
	char authenticate_str[40];
	char client_msg[MAXSIZE];
	char local_username[20];
	char local_password[20];
	char recv_username[20];
	char recv_password[20];
	char before_dot[20];
	char after_dot[20];
	char dfs_file_content[100];
	char dfs_file_content_copy[100];
	FILE *fp, *fp1;
	int unique_no;
	int recv_filelength;
	int recv_filename_length;
	char filename[15];
	char filename_copy[15];	
	char part_file_content[1000];
	char part_file_name[20];
	int folder_length;
	char cmd_folder[30];
	char recv_folder[10];

	//parse command line arguments
	if(argc != 3){
		printf ("USAGE: dfserver <DFS folder name> <port no>\n");
		exit(1);
	}

	strncpy(folder, argv[1], 20);
	port_no = atoi(argv[2]);
	unique_no = ((atoi(argv[2])-1) % NO_OF_CONNECTIONS) + 1;
	
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

	//function to be able to reuse socket numbers
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, (const void *)&temp , sizeof(int)); 

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

		if( (pid = fork()) == 0){
			printf("Process id is %d\n", getpid());
			//close(sock_listen);
			memset(recv_username, 0, strlen(recv_username));
			memset(recv_password, 0, strlen(recv_password));
			memset(authenticate_str, 0, strlen(authenticate_str));

			//receiving the length of the string having username and password
			if((bytes_read = recv(sock_connect, &recv_authenticate_length, sizeof(int), 0)) > 0){}

			//receiving the string having username and password	
			if((bytes_read = recv(sock_connect, authenticate_str, recv_authenticate_length, 0)) > 0){}


			temp_str = strtok(authenticate_str, " ");
			strcpy(recv_username, temp_str);

			//snipping off the \n in the first character of recv_username
			memmove(recv_username, recv_username + 1, strlen(recv_username));
			temp_str = strtok(NULL, " \n");	
			strcpy(recv_password, temp_str);	

			printf("Username is %s\n", recv_username);
			printf("Password is %s\n", recv_password);

			if((fp = fopen("dfs.conf", "r")) == NULL){
				printf("Config file not found.\n");
				exit(1);
			}

			while(!feof(fp)){
				fgets(dfs_file_content, 100, fp);
				if(strstr(dfs_file_content, recv_username) != NULL) {
					if(strstr(dfs_file_content, recv_password) != NULL){
						printf("Credentials match!\n");
						no = 1;
						send(sock_connect, &no, sizeof(int), 0);
						break;						
					}
					else{
						printf("Credentials dont match!\n");
						no = 0;
						send(sock_connect, &no, sizeof(int), 0);
					}
				}
				else{
					printf("Credentials do not match!\n");
					no = 0;
					send(sock_connect, &no, sizeof(int), 0);					
				}
			}

			fclose(fp);

			//receiving length of folder name
			recv(sock_connect, &folder_length, sizeof(int), 0);
			//printf("Received folder length is %d\n", folder_length);

			//receving the folder name eg. DFS1
			recv(sock_connect, &recv_folder, sizeof(int), 0);
    		char *p = recv_folder;
    		p[strlen(p)] = 0;			
			//printf("Received folder is %s\n", p);	

			//check if directory exists before opening
			DIR* dir = opendir(p);
			if (dir)
			{
    			printf("Directory exists %s\n", p);
    			closedir(dir);
			}
			else if (ENOENT == errno)
			{
    			/* Directory does not exist. */
    			sprintf(cmd_folder, "mkdir %s", p);
    			printf("Modified cmd is %s\n", cmd_folder);
    			system(cmd_folder);
    			printf("Directory %s does not exist.\n", p);
			}
			else
			{
				printf("Opendir() failed.\n");
    			/* opendir() failed for some other reason. */
			}

			//receiving length of file
			recv(sock_connect, &recv_filelength, sizeof(int), 0);
			printf("Received length is %d\n", recv_filelength);
			recv(sock_connect, &recv_filename_length, sizeof(int), 0);
			printf("Received filename length is %d\n", recv_filename_length);
			recv(sock_connect, filename, recv_filename_length, 0);
			printf("Filename received is %s\n", filename);
			strcpy(filename_copy, filename);
			memset(part_file_content, 0, sizeof(part_file_content));
			recv(sock_connect, part_file_content, recv_filelength, 0);
			//printf("Part file name content is %s\n", part_file_content);			
			sprintf(part_file_name, ".%s.%d", filename_copy, unique_no);
			printf("part file name is %s\n", part_file_name);
			strcat(part_file_content, "\0");

			//opening the file
			//giving relative path : DFS1/.foo.txt.1 and opening that by fopen
			sprintf(cmd_folder, "%s/%s", p, part_file_name);
			fp1 = fopen(cmd_folder, "w");
			fwrite(part_file_content, sizeof(char), recv_filelength+1, fp1);
			fclose(fp1);

		}
	}

	return 0;
}