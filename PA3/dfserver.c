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
#include <sys/mman.h>

#define MAX_CONF_SIZE 1000
#define NO_OF_CONNECTIONS 4
#define MAXSIZE 1000000
#define BACKLOG 10

struct sockaddr_in local, remote;
int sock_listen;   //socket descriptor
int sock_connect;  //socket descriptor
char folder[20];
int port_no;
int x;
static int *flag;
char cmd_folder1[100];


//return a folder after creating it - if it doesnt exist
char *return_directory(char *p, char *username){

	char temp2[100];
	sprintf(temp2, "%s/%s", p, username);
	DIR* dir = opendir(temp2);
	if (dir)
	{
		closedir(dir);
		return username;
	}
	else if (ENOENT == errno)
	{
		printf("p is %s\n", p);
		printf("username is %s\n", username);
    	/* Directory does not exist. */
    	sprintf(cmd_folder1, "cd %s && mkdir %s", p, username);
    	system(cmd_folder1);
    	return username;
	}
	else
	{
		/* opendir() failed for some other reason. */
		printf("Opendir() failed.\n");
	}	
}


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
	char dfs_file_content[100];
	char dfs_file_content_copy[100];
	FILE *fp, *fp1;
	int unique_no;
	int recv_filelength;
	int recv_filelength1;
	int recv_filename_length;
	char filename[15];
	char buff[30];	
	char part_file_content[1000];
	char part_file_name[20];
	int folder_length;
	char cmd_folder[30];
	char recv_folder[10];
	int flag_credentials = 0;

	//parse command line arguments
	if(argc != 3){
		printf ("USAGE: dfserver <DFS folder name> <port no>\n");
		exit(1);
	}

	strncpy(folder, argv[1], 20);
	port_no = atoi(argv[2]);
	unique_no = ((atoi(argv[2])-1) % NO_OF_CONNECTIONS) + 1;
	
	//creating shared memory for the flag variable to be accessed from any process
	flag = mmap(NULL, sizeof *flag, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*flag = 0;


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


			if((fp = fopen("dfs.conf", "r")) == NULL){
				printf("Config file not found.\n");
				exit(1);
			}

			//check credentials
			while(!feof(fp)){
				fgets(dfs_file_content, 100, fp);
				if(strstr(dfs_file_content, recv_username) != NULL) {
					if(strstr(dfs_file_content, recv_password) != NULL){
						printf("\nCredentials match!\n");
						flag_credentials = 1;
						no = 1;
						send(sock_connect, &no, sizeof(int), 0);
						break;						
					}
				}
			}

			fclose(fp);

			if(!flag_credentials){
				printf("\nCredentials do not match!\n");
				send(sock_connect, &no, sizeof(int), 0);
			}

			//receiving value of x
			recv(sock_connect, &x, sizeof(int), 0);
			printf("Received value of x is %d\n", x);

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
    			closedir(dir);
			}
			else if (ENOENT == errno)
			{
    			/* Directory does not exist. */
    			sprintf(cmd_folder, "mkdir %s", p);
    			//printf("Modified cmd is %s\n", cmd_folder);
    			system(cmd_folder);
			}
			else
			{
				/* opendir() failed for some other reason. */
				printf("Opendir() failed.\n");
			}

			char *recv_usr = recv_username;
			recv_usr[strlen(recv_usr)] = 0;

			if(x == 0){
				//file 1
				//receiving length of file
				recv(sock_connect, &recv_filelength, sizeof(int), 0);
				printf("Length of part file 1 is %d\n", recv_filelength);
				
				//receiving length of file
				recv(sock_connect, &recv_filelength1, sizeof(int), 0);
				printf("Length of part file 2 is %d\n", recv_filelength1);

				recv(sock_connect, &recv_filename_length, sizeof(int), 0);
				//printf("Received filename length is %d\n", recv_filename_length);
				recv(sock_connect, filename, recv_filename_length, 0);
				printf("Filename received is %s\n", filename);
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, unique_no);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");

				sprintf(buff, "cd %s && cd %s", p, return_directory(p, recv_usr));				
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength+1, fp1);
				fclose(fp1);


				//file 2
				
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength1, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, (unique_no+1)%4);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");


				sprintf(buff, "cd %s && cd %s", p, recv_usr);
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength1+1, fp1);
				fclose(fp1);

			}	
			else if(x == 1){
				//file 1
				//receiving length of file
				recv(sock_connect, &recv_filelength, sizeof(int), 0);
				printf("Length of part file 1 is %d\n", recv_filelength);
				
				//receiving length of file
				recv(sock_connect, &recv_filelength1, sizeof(int), 0);
				printf("Length of part file 2 is %d\n", recv_filelength1);

				recv(sock_connect, &recv_filename_length, sizeof(int), 0);
				//printf("Received filename length is %d\n", recv_filename_length);
				recv(sock_connect, filename, recv_filename_length, 0);
				printf("Filename received is %s\n", filename);
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, (unique_no+3)%4);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");


				sprintf(buff, "cd %s && cd %s", p, return_directory(p, recv_usr));			
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength+1, fp1);
				fclose(fp1);


				//file 2
				
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength1, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, unique_no);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");


				sprintf(buff, "cd %s && cd %s", p, recv_usr);
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength1+1, fp1);
				fclose(fp1);

			}	
			else if(x == 2){
				//file 1
				//receiving length of file
				recv(sock_connect, &recv_filelength, sizeof(int), 0);
				printf("Length of part file 1 is %d\n", recv_filelength);
				
				//receiving length of file
				recv(sock_connect, &recv_filelength1, sizeof(int), 0);
				printf("Length of part file 2 is %d\n", recv_filelength1);

				recv(sock_connect, &recv_filename_length, sizeof(int), 0);
				//printf("Received filename length is %d\n", recv_filename_length);
				recv(sock_connect, filename, recv_filename_length, 0);
				printf("Filename received is %s\n", filename);
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, (unique_no+2)%4);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");

				sprintf(buff, "cd %s && cd %s", p, return_directory(p, recv_usr));
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength+1, fp1);
				fclose(fp1);


				//file 2
				
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength1, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, (unique_no+3)%4);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");


				sprintf(buff, "cd %s && cd %s", p, recv_usr);
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength1+1, fp1);
				fclose(fp1);

			}	
			else if(x == 3){
				//file 1
				//receiving length of file
				recv(sock_connect, &recv_filelength, sizeof(int), 0);
				printf("Length of part file 1 is %d\n", recv_filelength);
				
				//receiving length of file
				recv(sock_connect, &recv_filelength1, sizeof(int), 0);
				printf("Length of part file 2 is %d\n", recv_filelength1);

				recv(sock_connect, &recv_filename_length, sizeof(int), 0);
				//printf("Received filename length is %d\n", recv_filename_length);
				recv(sock_connect, filename, recv_filename_length, 0);
				printf("Filename received is %s\n", filename);
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, (unique_no+1)%4);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");

				sprintf(buff, "cd %s && cd %s", p, return_directory(p, recv_usr));			
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength+1, fp1);
				fclose(fp1);


				//file 2
				
				memset(part_file_content, 0, sizeof(part_file_content));
				recv(sock_connect, part_file_content, recv_filelength1, 0);
				//printf("Part file name content is %s\n", part_file_content);			
				sprintf(part_file_name, ".%s.%d", filename, (unique_no+2)%4);
				printf("part file name is %s\n", part_file_name);
				strcat(part_file_content, "\0");


				sprintf(buff, "cd %s && cd %s", p, recv_usr);
				system(buff);
				sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
				fp1 = fopen(buff, "w");
				fwrite(part_file_content, sizeof(char), recv_filelength1+1, fp1);
				fclose(fp1);

			}	
			printf("At the end.\n");

		}
	}

	return 0;
}
