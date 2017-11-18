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
char cmd_folder1[100];
char options[20];
int options_length;
int menu_id = -1;
FILE *fp, *fp1;


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


//obtains file length in bytes
int file_length(FILE *fp)
{
    fseek(fp, 0, SEEK_END);
    int length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return length;
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
	char *p;
	int list_size = 0;
	char list_buf_contents[1000];
	char list_buf_contents_actual[1000];


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

		printf("\n\nWaiting for connections...\n");
		sock_connect = accept(sock_listen, (struct sockaddr*)&remote, (socklen_t *)&remote_len);
		if(sock_connect<0){
			perror("Cannot accept incoming connections.\n");
			exit(1);
		}	

		printf("\n\nAccepted a connection from %s\n", inet_ntoa(remote.sin_addr));


		options_length = 0;
		if(recv(sock_connect, &options_length, sizeof(options_length), 0) <= 0)
			continue;

		printf("Options length is %d\n", options_length);
		memset(options, 0, strlen(options));
		if(recv(sock_connect, options, options_length, 0) <=0)
			continue;
		printf("Options received is %s\n", options);

		if  (!strncmp(options, "LIST", 4))
			menu_id = 0;
		else if (!strncmp(options, "PUT ", 4))
			menu_id = 1;
		else if (!strncmp(options, "GET ", 4))
			menu_id = 2;
		else
			menu_id = 3;	


		if( (pid = fork()) == 0){
			printf("Process id is %d\n", getpid());
			//close(sock_listen);
			memset(recv_username, 0, strlen(recv_username));
			memset(recv_password, 0, strlen(recv_password));
			memset(authenticate_str, 0, strlen(authenticate_str));

			//receiving the length of the string having username and password
			if((bytes_read = recv(sock_connect, &recv_authenticate_length, sizeof(int), 0)) <= 0)
				continue;


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

			switch(menu_id){
				//LIST
				case 0:
						printf("LIST option.\n");
						memset(list_buf_contents_actual, 0, sizeof(list_buf_contents_actual));
						memset(list_buf_contents, 0, sizeof(list_buf_contents));

						//receiving length of folder name
						recv(sock_connect, &folder_length, sizeof(int), 0);
						//printf("Received folder length is %d\n", folder_length);

						//receving the folder name eg. DFS1
						memset(recv_folder, 0, sizeof(recv_folder));
						recv(sock_connect, &recv_folder, sizeof(int), 0);
    					p = recv_folder;
    					p[strlen(p)] = 0;

    					//command to print out all files and redirect to another file
						system("find . -name '.*' >list.txt");
						fp = fopen("list.txt", "r");
						list_size = file_length(fp);
						fread(list_buf_contents, sizeof(char), list_size, fp);
						fclose(fp);

						char * line = strtok(strdup(list_buf_contents), "\n");
						while(line) {
 	 						//printf("Line is :%s\n", line);
   							if(strstr(line, recv_username)!=NULL){
   								//printf("Line is :%s\n", line);
   								strcat(list_buf_contents_actual, line);
   								//strncpy(list_buf_contents_actual, line, strlen(line));
   								strcat(list_buf_contents_actual, "\n");
   							}
   							line  = strtok(NULL, "\n");
						}

						strcat(list_buf_contents_actual, "\0");

						//printf("Actual list is %s\n", list_buf_contents_actual);	
						int actual_length = strlen(list_buf_contents_actual);			
						//printf("Actual length is %d\n", actual_length);

						//sending out filesize first
						send(sock_connect, &actual_length, sizeof(int), 0);

						//sending out actual content
						send(sock_connect, list_buf_contents_actual, actual_length, 0);

						//deleting the temporary file - only 1 process deletes it
						if(unique_no == 1)
							system("rm list.txt");

						break;

				//PUT
				case 1:
						printf("PUT option.\n");
						//receiving value of x
						recv(sock_connect, &x, sizeof(int), 0);
						printf("Received value of x is %d\n", x);

						//receiving length of folder name
						recv(sock_connect, &folder_length, sizeof(int), 0);
						//printf("Received folder length is %d\n", folder_length);

						//receving the folder name eg. DFS1
						recv(sock_connect, &recv_folder, sizeof(int), 0);
    					p = recv_folder;
    					p[strlen(p)] = 0;			
						//printf("Received folder is %s\n", p);	

						//check if directory exists before opening
						DIR* dir = opendir(p);
						if (dir)
			    			closedir(dir);
						else if (ENOENT == errno){
			    			// Directory does not exist.
    						sprintf(cmd_folder, "mkdir %s", p);
    						system(cmd_folder);
						}
						else{
							// opendir() failed for some other reason. 
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
							sprintf(part_file_name, ".%s.%d", filename, unique_no%4);
							printf("part file name is %s\n", part_file_name);
							strcat(part_file_content, "\0");

							sprintf(buff, "cd %s && cd %s", p, return_directory(p, recv_usr));				
							system(buff);
							sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
							fp1 = fopen(buff, "w");
							fwrite(part_file_content, sizeof(char), recv_filelength, fp1);
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
							fwrite(part_file_content, sizeof(char), recv_filelength1, fp1);
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
							fwrite(part_file_content, sizeof(char), recv_filelength, fp1);
							fclose(fp1);

							//file 2
				
							memset(part_file_content, 0, sizeof(part_file_content));
							recv(sock_connect, part_file_content, recv_filelength1, 0);
							//printf("Part file name content is %s\n", part_file_content);			
							sprintf(part_file_name, ".%s.%d", filename, unique_no%4);
							printf("part file name is %s\n", part_file_name);
							strcat(part_file_content, "\0");


							sprintf(buff, "cd %s && cd %s", p, recv_usr);
							system(buff);
							sprintf(buff, "%s/%s/%s", p, recv_usr, part_file_name);
							fp1 = fopen(buff, "w");
							fwrite(part_file_content, sizeof(char), recv_filelength, fp1);
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
							fwrite(part_file_content, sizeof(char), recv_filelength, fp1);
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
							fwrite(part_file_content, sizeof(char), recv_filelength, fp1);
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
							fwrite(part_file_content, sizeof(char), recv_filelength, fp1);
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
							fwrite(part_file_content, sizeof(char), recv_filelength, fp1);
							fclose(fp1);
						}

						break;
	
				case 2:
						printf("GET option.\n");
						printf("x is %d\n", x);
						char req_filename[20];
						char *ptr_req_filename;
						strcpy(req_filename, options);
						strtok(req_filename, " ");
						ptr_req_filename = strtok(NULL, " \n");
						printf("Requested file is %s\n", ptr_req_filename);


						//receiving length of folder name
						recv(sock_connect, &folder_length, sizeof(int), 0);

						//receving the folder name eg. DFS1
						memset(recv_folder, 0, sizeof(recv_folder));
						recv(sock_connect, &recv_folder, sizeof(int), 0);
    					p = recv_folder;
    					p[strlen(p)] = 0;


						char temp2[100], temp3[100], temp4[100], line_buf[100], file_path[100];
						char list_files[100];
						static int count_instances = 0;
						int len_name = 0;
						char data_part_file[MAXSIZE];
						int len_file_read = 0;
						int length_of_file = 0;
						memset(list_files, 0, sizeof(list_files));
						sprintf(temp2, "%s/%s", p, recv_username);
						printf("recv user is %s\n", recv_username);
						printf("temp2 is %s\n", temp2);
						DIR* dir1 = opendir(temp2);
						if (dir1)
						{
							sprintf(temp3, "cd %s/%s && find . -name '.*' >internal_list.txt", p, recv_username);
							//command to print out all files and redirect to another file
							system(temp3);
							sprintf(temp4, "%s/%s/internal_list.txt", p, recv_username);
							fp = fopen(temp4, "r");
							while(fgets(line_buf, 150, fp)){
								if(strlen(line_buf)>2){
									memmove(line_buf, line_buf + 2, strlen(line_buf));
									line_buf[strlen(line_buf)-1] = 0;
									if(strstr(line_buf, ptr_req_filename)!=NULL){
										//sprintf(file_path, "%s/%s/%s", p, recv_username, ptr_req_filename);
										count_instances++;
										printf("%s\n", line_buf);
									}
								}
							}
							fclose(fp);

							//sending the number of part files to be sent
							send(sock_connect, &count_instances, sizeof(int), 0);
							count_instances = 0;
							//sending out the name and file contents
							fp = fopen(temp4, "r");
							while(fgets(line_buf, 150, fp)){
								if(strlen(line_buf)>2){
									memmove(line_buf, line_buf + 2, strlen(line_buf));
									line_buf[strlen(line_buf)-1] = '\0';
									memset(data_part_file, 0, strlen(data_part_file));
									if(strstr(line_buf, ptr_req_filename)!=NULL){
										sprintf(file_path, "%s/%s/%s", p, recv_username, line_buf);
										len_name = strlen(line_buf);
										//sending the length of filename
										send(sock_connect, &len_name, sizeof(len_name), 0);
										//sending filename
										printf("File name to send is %s\n", line_buf);
										send(sock_connect, line_buf, len_name, 0);
										fp1 = fopen(file_path, "r");
										length_of_file = file_length(fp1);
										length_of_file++;
										len_file_read = fread(data_part_file, sizeof(char), length_of_file, fp1);
										fclose(fp1);
										data_part_file[len_file_read] = '\0';
										printf("data part is %s and length is %d\n", data_part_file, len_file_read);
										//sending the length of file
										send(sock_connect, &len_file_read, sizeof(int), 0);
										printf("length of data sent is %d\n", len_file_read);
										//len_file_read = 0;
										//sending the part file
										send(sock_connect, data_part_file, len_file_read, 0);
										printf("data sent out: %s\n", data_part_file);
										//memset(data_part_file, 0, strlen(data_part_file));
									}		
								}
							}
							fclose(fp);
							closedir(dir1);
						}
						else if (ENOENT == errno)
						{
    						/* Directory does not exist. */
    						printf("Directory does not exist.\n");
						}


						break;

				default:
						printf("Did not match any of the options.\n");

			}//end of switch case	
			printf("At the end.\n");

		}//end of fork call
	}//end of while loop

	return 0;
}
