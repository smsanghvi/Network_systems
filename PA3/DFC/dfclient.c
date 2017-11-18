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
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <openssl/md5.h>

#define MAX_CONF_SIZE 1000
#define BUFFER_SIZE 1000000
#define NO_OF_CONNECTIONS 4

static char folder[NO_OF_CONNECTIONS][10];
static char ip[NO_OF_CONNECTIONS][20];
int port[NO_OF_CONNECTIONS];
char username[20];
char username_copy[20];
int username_length;
char password[20];
char options[20];
char combined_file[BUFFER_SIZE];
char combined_filename[20];
int combined_length;
char *str;
int16_t menu_id = -1;	
int count = 0;
int x = 0;
int sockfd[NO_OF_CONNECTIONS];
struct sockaddr_in servaddr[NO_OF_CONNECTIONS];
char storage_buf[NO_OF_CONNECTIONS][BUFFER_SIZE];
int options_length = 0;
int recv_list_size[NO_OF_CONNECTIONS];
char recv_list_buffer[NO_OF_CONNECTIONS][200];
char recv_list_buffer_copy[200];
char buffer_all_files[200] = {0};
char buffer_part[NO_OF_CONNECTIONS][BUFFER_SIZE];
int length_part_file[NO_OF_CONNECTIONS];
char list_unique_strings[10][15];
int list_unique_nums[10];
int recv_file_count[NO_OF_CONNECTIONS];


//for md5
int md5_n;
MD5_CTX md5_c;
char md5_buf[512];
ssize_t md5_bytes;
unsigned char md5_out[MD5_DIGEST_LENGTH];

char buf_conf[MAX_CONF_SIZE];


//computing md5 sum of file and returning modulus of 4
int md5sum_mod(FILE *fp){
	MD5_Init(&md5_c);
    md5_bytes = fread(md5_buf, 1, 512, fp);
    while(md5_bytes > 0)
    {
            MD5_Update(&md5_c, md5_buf, md5_bytes);
            md5_bytes = fread(md5_buf, 1, 512, fp);
    }

    MD5_Final(md5_out, &md5_c);

    return md5_out[0] % 4;
}


//parsing dfc.conf file
void parse_conf(FILE *fp){
	while(count<6){
		//reading in a line at a time
		fgets(buf_conf, MAX_CONF_SIZE, fp);
		str = strtok(buf_conf, " ");
		switch(count){
			case 0:	str = strtok(NULL, " ");
					strcpy(folder[0], str);
					str = strtok(NULL, ":");
					strcpy(ip[0], str);
					port[0] = atoi(strtok(NULL, " "));
					break;
			case 1:	str = strtok(NULL, " ");
					strcpy(folder[1], str);
					str = strtok(NULL, ":");
					strcpy(ip[1], str);
					port[1] = atoi(strtok(NULL, " "));
					break;
			case 2:	str = strtok(NULL, " ");
					strcpy(folder[2], str);
					str = strtok(NULL, ":");
					strcpy(ip[2], str);
					port[2] = atoi(strtok(NULL, " "));
					break;
			case 3:	str = strtok(NULL, " ");
					strcpy(folder[3], str);
					str = strtok(NULL, ":");
					strcpy(ip[3], str);
					port[3] = atoi(strtok(NULL, " "));
					break;					
			case 4: str = strtok(NULL, " \n");
					strcpy(username, str);
					strcpy(username_copy, str);
					break;
			case 5: str = strtok(NULL, " \n");
					strcpy(password, str);
					break;
		}
		++count;
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


//creating the 4 sockets, populating them and connecting to server
int create_sockets(){
	int i;
 	for(i = 0; i < NO_OF_CONNECTIONS; i++){
 		//making socket function calls
 		if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
  			perror("Problem in creating the socket");
  			return 1;
 		}	

 		//populating the structures
 		memset(&servaddr[i], 0, sizeof(servaddr[i]));
 		servaddr[i].sin_family = AF_INET;
		servaddr[i].sin_addr.s_addr= inet_addr(ip[i]);
 		servaddr[i].sin_port =  htons(port[i]); //convert to big-endian order

 		//Connection of the client to the socket 
 		if (connect(sockfd[i], (struct sockaddr *) &servaddr[i], sizeof(servaddr[i])) == -1) {
  			perror("Problem in connecting to the server");
  			return 1;
 		}
 	}

 	return 0;
}


//comparision function used for qsort
int cmpfunc( const void *a, const void *b) {
  return *(char*)a - *(char*)b;
}


int main(int argc, char **argv){
	int i;
	int bytes_read[4];
 	int resp[4];
 	int length_authenticate;
 	char concat_authenticate[40];
 	char *temp_str;
 	char filename[20];
 	int length_of_file;
 	int flag_authenticate = 0;
 	int folder_length;


	//basic check for the arguments
 	if (argc != 2) {
  		printf("Usage: dfclient <conf file>\n"); 
  		exit(1);
 	}

 	FILE *fp;

	while(1){
		//opening the conf file
		fp = fopen(argv[1], "r");
		//printf("Parsing the client configuration file...\n");
		parse_conf(fp);
		fclose(fp);

		strcpy(concat_authenticate, "\n");
		strcat(concat_authenticate, username);
		strcat(concat_authenticate, " ");
		strcat(concat_authenticate, password);
		length_authenticate = strlen(concat_authenticate);
		//printf("Concat is %s\n", concat_authenticate);
		//printf("Strlen is %d\n", length_authenticate);

 		//creating multiple sockets
 		if(create_sockets()){
 			printf("Sockets not created. Exiting...\n");
 			exit(1);
 		}


		printf("\n-------------------------------------");
		printf("\nEnter one of these options:\n");
		printf("LIST\n");	
		printf("PUT [file_name]\n");	
		printf("GET [file_name]\n\n");	

		//getting input from user
		memset(options, 0, sizeof(options));
		fgets(options, 20, stdin);

	//while(1){
		username_length = strlen(username);

		if  (!strncmp(options, "LIST", 4))
			menu_id = 0;
		else if (!strncmp(options, "PUT ", 4))
			menu_id = 1;
		else if (!strncmp(options, "GET ", 4))
			menu_id = 2;
		else
			menu_id = 3;	

		//sending out the options
		for(i=0; i< NO_OF_CONNECTIONS; i++){
			options_length = strlen(options);
			send(sockfd[i], &options_length, sizeof(options_length), 0);
			send(sockfd[i], options, options_length, 0);
		}				

		//sending out user credentials
		for(i=0; i<NO_OF_CONNECTIONS; i++){					
			send(sockfd[i], &length_authenticate, sizeof(int), 0);
		}

		for(i=0; i<NO_OF_CONNECTIONS; i++){					
			send(sockfd[i], concat_authenticate, length_authenticate, 0);
		}

		//waiting for correct credentials reply from servers
		for(i=0; i<NO_OF_CONNECTIONS; i++){					
			if((bytes_read[i] = recv(sockfd[i], &resp[i], sizeof(int), 0)) > 0){}	
			if(resp[i] == 1){
				printf("Authentication successful with server %d\n", i);
				flag_authenticate = 0;
			}
			else{
				printf("Invalid Username/Password. Please try again.\n");
				flag_authenticate = 1;
				break;
			}
		}


		if(flag_authenticate)
			continue;

		switch(menu_id){
			//LIST
			case 0:
					printf("\n");
					for(i=0; i<NO_OF_CONNECTIONS;i++){
						folder_length = strlen(folder[i]);				
						//sending the length of the folder
						send(sockfd[i], &folder_length, sizeof(int), 0);
						//sending the folder name
						send(sockfd[i], folder[i], folder_length, 0);
					}

					for(i=0; i<NO_OF_CONNECTIONS; i++){
						memset(recv_list_buffer[i], 0, sizeof(recv_list_buffer[i]));
						recv(sockfd[i], &recv_list_size[i], sizeof(int), 0);
						recv(sockfd[i], recv_list_buffer[i], recv_list_size[i], 0);
					}

					printf("LIST OF SERVER FILES: -\n");
					printf("--------------------------------\n");
					printf("%s\n\n",recv_list_buffer[3]);	

					strcpy(recv_list_buffer_copy, recv_list_buffer[3]);
					//printf("%s\n\n", recv_list_buffer_copy);


					char *str_list_temp;

					//creating a buffer of all the files called buffer_all_files
					str_list_temp = strtok(strdup(recv_list_buffer_copy), ".");
					memset(buffer_all_files, 0, sizeof(buffer_all_files));

					int count_line = 0;
					
					while(str_list_temp != '\0' && str_list_temp != NULL){
						if(count_line == 0){
							str_list_temp = strtok(NULL, "\n");
							strcat(buffer_all_files, ".");
							strcat(buffer_all_files, str_list_temp);
   							strcat(buffer_all_files, "\n");							
						}
						else{
							str_list_temp = strtok(NULL, "/");
						}
						
   						if(count_line == 0){
							str_list_temp = strtok(NULL, "/");
							str_list_temp = strtok(NULL, "/");
							str_list_temp = strtok(NULL, "/");
   						}
   						else if(count_line == 1){
  							str_list_temp = strtok(NULL, "\n");
  							char temp1[20];
  							int i = 0;
  							strcpy(temp1, str_list_temp);
  							while(*(temp1+i) != '/'){
  								i++;
  							}
  							i++;
   							strcat(buffer_all_files, temp1+i);
   							strcat(buffer_all_files, "\n");   							
   						}
   						else{
   							str_list_temp = strtok(NULL, "\n");
   							strcat(buffer_all_files, str_list_temp);
   							strcat(buffer_all_files, "\n");
   						}

						if(count_line == 0){
							str_list_temp = strtok(NULL, "\n");
							strcat(buffer_all_files, str_list_temp);
   							strcat(buffer_all_files, "\n");									
						}	   					
						else if((str_list_temp = strtok(NULL, "/")) == NULL){
							break;
   						}

						if((str_list_temp = strtok(NULL, "/")) == NULL){
							break;
						}
					
						count_line++;
					}

					strcat(buffer_all_files, "\0");


					printf("Buffer all files is :\n");
					printf("--------------------------\n");
					printf("%s\n", buffer_all_files);

					FILE *fp3 = fopen("temp.txt", "w");
					fwrite(buffer_all_files, sizeof(char), strlen(buffer_all_files), fp3);
					fclose(fp3);

					/*char line1[100];
					char line2[100];
					char line_temp1[100];
					int count = 0;
					char line_temp2[100];
					char unique_arr[10][100];
					int unique_arr_cnt[10];
					int len1, len2, last_line1, last_line2;
					fp3 = fopen("temp.txt", "r");
					while(fgets(line1, 150, fp3)){
						len1 = strlen(line1);
						fgets(line2, 150, fp3 );
						len2 = strlen(line2);
						printf("Last character of line1 is %c\n", line1[len1-2]);
						memset(line_temp1, 0, strlen(line_temp1));
						memcpy(line_temp1, line1, len1 - 2);
						memset(line_temp2, line2, len2 - 2);
						if(!strcmp(line_temp1, line_temp2)){
							if(line1[len1-2] != line2[len2-2]){
								unique_arr[]
							}
						}
						else{
							strcpy(unique_arr[count], line1);
							//strcpy(unique_arr[count], line1);
							count++; 
							strcpy(unique_arr[count], line2);
							count++;
						}
						printf("line temp 1 is %s\n", line_temp1);
					}
					fclose(fp3);*/



					break;
			//PUT
			case 1:
					printf("\n");

					temp_str = strtok(options, " ");
					temp_str = strtok(NULL, " \n");
					strcpy(filename, temp_str);

					//check if file exists
					if( access( filename, F_OK ) == -1 ) {
						printf("File does not exist.\nExiting...");
						exit(1);
					}

					//obtaining file length in bytes
					fp = fopen(filename, "r");
					length_of_file = file_length(fp);
					x = md5sum_mod(fp);
					printf("x is %d\n", x);
					printf("Length of %s is %d\n", filename, length_of_file);
					fclose(fp);

					int sum = 0;

					//setting the lengths of part files
					for(i = 0; i < NO_OF_CONNECTIONS; i++){
						length_part_file[i] = length_of_file/NO_OF_CONNECTIONS;
						if(i < (NO_OF_CONNECTIONS)){
							sum += length_part_file[i];
						}
						length_part_file[NO_OF_CONNECTIONS - 1] += (length_of_file - sum); 
					}
	
					int temp_length = strlen(filename);

					//storing all the file content values into 4 different buffers
					fp = fopen(filename, "r");
					for(i = 0; i < NO_OF_CONNECTIONS; i++){
						length_part_file[i] = fread(storage_buf[i], sizeof(char), length_part_file[i], fp);
					}
					fclose(fp);

					for(i = 0; i < NO_OF_CONNECTIONS; i++){
						//sending out the value of x
						send(sockfd[i], &x, sizeof(int), 0);
						folder_length = strlen(folder[i]);
						//length_part_file[i] = fread(buffer_part[i], sizeof(char), length_part_file[i], fp);
						//sending the length of the folder
						send(sockfd[i], &folder_length, sizeof(int), 0);
						//sending the folder name
						send(sockfd[i], folder[i], folder_length, 0);

						if(x==0){
							//sending the length of the part file
							printf("Length of part file %d is %d\n", i, length_part_file[i]);
							send(sockfd[i], &length_part_file[i], sizeof(int), 0);

							//sending the length of the part file
							printf("Length of part file %d is %d\n", (i+1)%4, length_part_file[(i+1)%4]);
							send(sockfd[i], &length_part_file[(i+1)%4], sizeof(int), 0);

							//sending the length of the filename
							send(sockfd[i], &temp_length, sizeof(int), 0);
							//sending the name of the file 
							send(sockfd[i], filename, strlen(filename), 0);
							
							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[i]);
							send(sockfd[i], buffer_part[i], length_part_file[i] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+1)%4]);
							send(sockfd[i], buffer_part[i], length_part_file[(i+1)%4] + 1, 0);	
										
						}
						else if(x==1){
							//sending the length of the part file
							printf("Length of part file %d is %d\n", (i+3)%4, length_part_file[(i+3)%4]);
							send(sockfd[i], &length_part_file[(i+3)%4], sizeof(int), 0);

							//sending the length of the part file
							printf("Length of part file %d is %d\n", i, length_part_file[i]);
							send(sockfd[i], &length_part_file[i], sizeof(int), 0);

							//sending the length of the filename
							send(sockfd[i], &temp_length, sizeof(int), 0);
							//sending the name of the file 
							send(sockfd[i], filename, strlen(filename), 0);
							
							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+3)%4]);
							send(sockfd[i], buffer_part[i], length_part_file[(i+3)%4] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[i]);
							send(sockfd[i], buffer_part[i], length_part_file[i] + 1, 0);	
										
						}
						else if(x==2){
							//sending the length of the part file
							printf("Length of part file %d is %d\n", (i+2)%4, length_part_file[(i+2)%4]);
							send(sockfd[i], &length_part_file[(i+2)%4], sizeof(int), 0);

							//sending the length of the part file
							printf("Length of part file %d is %d\n", (i+3)%4, length_part_file[(i+3)%4]);
							send(sockfd[i], &length_part_file[(i+3)%4], sizeof(int), 0);

							//sending the length of the filename
							send(sockfd[i], &temp_length, sizeof(int), 0);
							//sending the name of the file 
							send(sockfd[i], filename, strlen(filename), 0);
							
							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+2)%4]);
							send(sockfd[i], buffer_part[i], length_part_file[(i+2)%4] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+3)%4]);
							send(sockfd[i], buffer_part[i], length_part_file[(i+3)%4] + 1, 0);	
										
						}
						else if(x==3){
							//sending the length of the part file
							printf("Length of part file %d is %d\n", (i+1)%4, length_part_file[(i+1)%4]);
							send(sockfd[i], &length_part_file[(i+1)%4], sizeof(int), 0);

							//sending the length of the part file
							printf("Length of part file %d is %d\n", (i+2)%4, length_part_file[(i+2)%4]);
							send(sockfd[i], &length_part_file[(i+2)%4], sizeof(int), 0);

							//sending the length of the filename
							send(sockfd[i], &temp_length, sizeof(int), 0);
							//sending the name of the file 
							send(sockfd[i], filename, strlen(filename), 0);
							
							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+1)%4]);
							send(sockfd[i], buffer_part[i], length_part_file[(i+1)%4] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+2)%4]);
							send(sockfd[i], buffer_part[i], length_part_file[(i+2)%4] + 1, 0);	
										
						}
					
					}

					break;
			//GET
			case 2:
					printf("\n");
					int rec_filename_length[NO_OF_CONNECTIONS];
					char rec_filename[NO_OF_CONNECTIONS][20];
					int rec_file_length[NO_OF_CONNECTIONS];
					char rec_file_content[NO_OF_CONNECTIONS][BUFFER_SIZE];
					char temp_arr[BUFFER_SIZE];
					for(i=0; i<NO_OF_CONNECTIONS;i++){
						folder_length = strlen(folder[i]);				
						//sending the length of the folder
						send(sockfd[i], &folder_length, sizeof(int), 0);
						//sending the folder name
						send(sockfd[i], folder[i], folder_length, 0);
					}

					//receiving the number of unique files from each server
					for(i=0; i<NO_OF_CONNECTIONS;i++){
						recv(sockfd[i], &recv_file_count[i], sizeof(int), 0);
						printf("File count for server %d is %d\n", i, recv_file_count[i] );
					}

					for(i=0; i<NO_OF_CONNECTIONS;i++){
						//receive the length of filename
						//while(recv_file_count[i]>0){
							memset(rec_filename[i], 0, sizeof(rec_filename[i]));
							memset(rec_file_content[i], 0, sizeof(rec_file_content[i]));
							recv(sockfd[i], &rec_filename_length[i], sizeof(int), 0);
							if(recv(sockfd[i], rec_filename[i], rec_filename_length[i], 0) > 0){
								printf("Filename for %d and instance %d is %s\n", i, recv_file_count[i], rec_filename[i]);
								//check if file exists
								if( access( rec_filename[i], F_OK ) == -1 ) {
									//file does not exist
									recv(sockfd[i], &rec_file_length[i], sizeof(int), 0);
									printf("File length for %d and instance %d is %d\n", i, recv_file_count[i], rec_file_length[i]);
									recv(sockfd[i], rec_file_content[i], rec_file_length[i], 0);
									rec_file_content[i][rec_file_length[i]] = '\0'; 
									fp = fopen(rec_filename[i], "w");
									fwrite(rec_file_content[i], sizeof(char), rec_file_length[i], fp);
									fclose(fp);
								}
								else{
									//file already exists
									printf("File %s already exists.\n", rec_filename[i]);
									memset(temp_arr, 0, sizeof(temp_arr));
									fp = fopen(rec_filename[i], "r");
									fread(temp_arr, sizeof(char), rec_file_length[i], fp);
									fclose(fp);
									if(!isprint(temp_arr[0])){
										printf("%s has unprintable characters. Replacing it...\n", rec_filename[i]);
										char rm_buf[100];
										sprintf(rm_buf, "rm %s", rec_filename[i]);
										system(rm_buf);
										recv(sockfd[i], &rec_file_length[i], sizeof(int), 0);
										printf("File length for %d and instance %d is %d\n", i, recv_file_count[i], rec_file_length[i]);
										recv(sockfd[i], rec_file_content[i], rec_file_length[i], 0);
										rec_file_content[i][rec_file_length[i]] = '\0'; 
										fp = fopen(rec_filename[i], "w");
										fwrite(rec_file_content[i], sizeof(char), rec_file_length[i], fp);
										fclose(fp);									
									}							
								}
								printf("-------------------------------------\n");
							}

							recv_file_count[i]--;

							if(recv_file_count[i] > 0){
								memset(rec_filename[i], 0, sizeof(rec_filename[i]));
								memset(rec_file_content[i], 0, sizeof(rec_file_content[i]));
								recv(sockfd[i], &rec_filename_length[i], sizeof(int), 0);
								if(recv(sockfd[i], rec_filename[i], rec_filename_length[i], 0) > 0){
									printf("AGAIN!!!\n");
									printf("Filename for %d and instance %d is %s\n", i, recv_file_count[i], rec_filename[i]);
									//check if file exists
									if( access( rec_filename[i], F_OK ) == -1 ) {
										//file does not exist
										recv(sockfd[i], &rec_file_length[i], sizeof(int), 0);
										printf("File length for %d and instance %d is %d\n", i, recv_file_count[i], rec_file_length[i]);
										rec_file_content[i][rec_file_length[i]] = '\0'; 
										fp = fopen(rec_filename[i], "w");
										fwrite(rec_file_content[i], sizeof(char), rec_file_length[i], fp);
										fclose(fp);
									}
									else{
										//file exists
										printf("File %s already exists.\n", rec_filename[i]);
										memset(temp_arr, 0, sizeof(temp_arr));
										fp = fopen(rec_filename[i], "r");
										fread(temp_arr, sizeof(char), rec_file_length[i], fp);
										fclose(fp);
										if(!isprint(temp_arr[0])){
											printf("%s has unprintable characters\n", rec_filename[i]);										
											char rm_buf[100];
											sprintf(rm_buf, "rm %s", rec_filename[i]);
											system(rm_buf);
											recv(sockfd[i], &rec_file_length[i], sizeof(int), 0);
											printf("File length for %d and instance %d is %d\n", i, recv_file_count[i], rec_file_length[i]);
											recv(sockfd[i], rec_file_content[i], rec_file_length[i], 0);
											rec_file_content[i][rec_file_length[i]] = '\0'; 
											fp = fopen(rec_filename[i], "w");
											fwrite(rec_file_content[i], sizeof(char), rec_file_length[i], fp);
											fclose(fp);											
										}										
									}
									printf("-------------------------------------\n");
								}							
							}				
						//}					
					}

					//printf("options is %s\n", options);
					char *name;
					char name_arr[20], temp_sprintf[20], rec_name0[20], rec_name1[20], rec_name2[20], rec_name3[20];
					int len_0, len_1, len_2, len_3, temp_cnt=0;
					strtok(options, " ");
					name = strtok(NULL, " \n");
					strcpy(name_arr, name);
					printf("name is %s\n", name_arr);
					sprintf(temp_sprintf, ".%s", name_arr);
					sprintf(rec_name0, "%s.0", temp_sprintf);
					sprintf(rec_name1, "%s.1", temp_sprintf);
					sprintf(rec_name2, "%s.2", temp_sprintf);
					sprintf(rec_name3, "%s.3", temp_sprintf);

					printf("rec_name0 is %s \n", rec_name0);
					printf("rec_name1 is %s \n", rec_name1);
					printf("rec_name2 is %s \n", rec_name2);
					printf("rec_name3 is %s \n", rec_name3);
					//recombining the file
					char temp_parts[BUFFER_SIZE];
					//for(i=0; i<NO_OF_CONNECTIONS;i++){
					printf("x is %d\n", x);
						//if(x == 3){
							/*if( (access( rec_name0, F_OK ) != -1 ) && \
							(access( rec_name1, F_OK ) != -1 ) && (access( rec_name2, F_OK ) != -1 ) \
							&& (access( rec_name3, F_OK ) != -1 )){ 
								//file exists
								printf("Complete file exists.\n");
								fp = fopen(rec_name1, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_1 = file_length(fp);
								fread(temp_parts, sizeof(char), len_1, fp );
								printf("1 is: %s\n", temp_parts);
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name0, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_2 = file_length(fp);								
								fread(temp_parts, sizeof(char), len_2, fp );
								printf("2 is: %s\n", temp_parts);								
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name3, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_3 = file_length(fp);								
								fread(temp_parts, sizeof(char), len_3, fp );
								printf("3 is: %s\n", temp_parts);
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name0, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_0 = file_length(fp);
								fread(temp_parts, sizeof(char), len_0, fp );
								printf("4 is: %s\n", temp_parts);
								strcat(combined_file, temp_parts);
								fclose(fp);
								strcat(combined_file, "\0");

								printf("Total file is :\n");
								printf("-------------------------------------------\n");
								printf("%s\n", combined_file);
								//fp = fopen()

							}
							else{
								printf("Incomplete.\n");
							}*/
						//}
						if(x == 1){
							if( (access( rec_name0, F_OK ) != -1 ) && \
							(access( rec_name1, F_OK ) != -1 ) && (access( rec_name2, F_OK ) != -1 ) \
							&& (access( rec_name3, F_OK ) != -1 )){ 
								//file exists
								temp_cnt = 0;
								memset(combined_file, 0, sizeof(combined_file));
								printf("Complete file exists.\n");
								fp = fopen(rec_name1, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_2 = file_length(fp);								
								len_2 = fread(temp_parts, sizeof(char), len_2, fp );
								printf("2 is: %s and len2 is %d\n", temp_parts, len_2);								
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name2, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_3 = file_length(fp);								
								len_3 = fread(temp_parts, sizeof(char), len_3 , fp );
								printf("3 is: %s and len3 is %d\n", temp_parts, len_3);
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name3, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_0 = file_length(fp);
								len_0 = fread(temp_parts, sizeof(char), len_0, fp );
								printf("4 is: %s and len0 is %d\n", temp_parts, len_0);
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name0, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_1 = file_length(fp);
								len_1 = fread(temp_parts, sizeof(char), len_1, fp );
								if(temp_parts[0]=='\0'){
									temp_cnt++;
								}
								printf("1 is: %s and len1 is %d\n", (temp_parts+temp_cnt), len_1);
								strcat(combined_file, (temp_parts+temp_cnt));
								fclose(fp);
								strcat(combined_file, "\0");

								//printf("Total file is :\n");
								//printf("-------------------------------------------\n");
								printf("%s\n", combined_file);
								fp = fopen("modified_text.txt", "w");
								fwrite(combined_file, sizeof(char), strlen(combined_file), fp);
								fclose(fp);

							}
							else{
								printf("Incomplete.\n");
							}							
						}
						else if(x == 2){
							if( (access( rec_name0, F_OK ) != -1 ) && \
							(access( rec_name1, F_OK ) != -1 ) && (access( rec_name2, F_OK ) != -1 ) \
							&& (access( rec_name3, F_OK ) != -1 )){ 
								//file exists
								temp_cnt = 0;
								memset(combined_file, 0, sizeof(combined_file));
								printf("Complete file exists.\n");
								fp = fopen(rec_name1, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_2 = file_length(fp);								
								len_2 = fread(temp_parts, sizeof(char), len_2, fp );
								printf("1 is: %s and len2 is %d\n", temp_parts, len_2);								
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name2, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_3 = file_length(fp);								
								len_3 = fread(temp_parts, sizeof(char), len_3 , fp );
								printf("2 is: %s and len3 is %d\n", temp_parts, len_3);
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name3, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_0 = file_length(fp);
								len_0 = fread(temp_parts, sizeof(char), len_0, fp );
								printf("3 is: %s and len0 is %d\n", temp_parts, len_0);
								strcat(combined_file, temp_parts);
								fclose(fp);
								fp = fopen(rec_name0, "r");
								memset(temp_parts, 0, sizeof(temp_parts));
								len_1 = file_length(fp);
								len_1 = fread(temp_parts, sizeof(char), len_1, fp );
								printf("0 is: %s and len1 is %d\n", (temp_parts), len_1);
								strcat(combined_file, (temp_parts+temp_cnt));
								fclose(fp);
								strcat(combined_file, "\0");

								//printf("Total file is :\n");
								//printf("-------------------------------------------\n");
								printf("%s\n", combined_file);
								fp = fopen("modified_text.txt", "w");
								fwrite(combined_file, sizeof(char), strlen(combined_file), fp);
								fclose(fp);

							}
							else{
								printf("Incomplete.\n");
							}							
						}
					//}

					break;
			//default
			case 3:
					printf("\n");
		
					printf("Enter option correctly.\n");		
		}

	/*printf("\n-------------------------------------");
	printf("\nEnter one of these options:\n");
	printf("LIST\n");	
	printf("PUT [file_name]\n");	
	printf("GET [file_name]\n\n");	

	//getting input from user
	memset(options, 0, sizeof(options));
	fgets(options, 20, stdin);*/
		//break;
	}


	return 0;
}