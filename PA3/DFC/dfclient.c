/*****************************************************************************
​ * ​ ​ Copyright​ ​ (C)​ ​ 2017​ ​ by​ ​ Snehal Sanghvi
​ *
​ * ​ ​  Users​ ​ are  ​ permitted​ ​ to​ ​ modify​ ​ this​ ​ and​ ​ use​ ​ it​ ​ to​ ​ learn​ ​ about​ ​ the​ ​ field​ ​ of​ ​ embedded
​ * ​ ​ software.​ ​ Snehal Sanghvi​ ​ and​ ​ the​ ​ University​ ​ of​ ​ Colorado​ ​ are​ ​ not​ ​ liable​ ​ for​ ​ any​ ​ misuse​ ​ of​ ​ this​ ​ material.
​ *
*****************************************************************************/
/**
​ * ​ ​ @file​ ​ dfclient.c
​ * ​ ​ @brief​ ​ Source file having the client implementation of the distributed file system
​​ * ​ ​ @author​ ​ Snehal Sanghvi
​ * ​ ​ @date​ ​ November ​ 21 ​ 2017
​ * ​ ​ @version​ ​ 1.0
​ *   @compiler used to process code: GCC compiler
 *	 @functionality implemented: 
 	 Created the client side of the Distributed File System
​ */

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

//macro to turn on/off the encryption featuer
//#define ENCRYPTION 1

int flag = -1;
static char folder[NO_OF_CONNECTIONS][10];
static char ip[NO_OF_CONNECTIONS][20];
int port[NO_OF_CONNECTIONS];
char username[20];
char username_copy[20];
int username_length;
char password[20], options[20];
char key[20];
int keyLen;
int dataLen;
char combined_file[BUFFER_SIZE];
char combined_filename[20];
int combined_length, options_length = 0;
char *str;
static int active = NO_OF_CONNECTIONS;
int16_t menu_id = -1;	
int count = 0, x = 0;;
int sockfd[NO_OF_CONNECTIONS] = {-1};
struct sockaddr_in servaddr[NO_OF_CONNECTIONS];
char storage_buf[NO_OF_CONNECTIONS][BUFFER_SIZE];
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
	active = NO_OF_CONNECTIONS;
 	for(i = 0; i < NO_OF_CONNECTIONS; i++){
 		//making socket function calls
 		sockfd[i] = -1;
 		if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
  			perror("Problem in creating the socket");
  			active --;
  			flag = i;
  			continue;
 		}	

 		//populating the structures
 		memset(&servaddr[i], 0, sizeof(servaddr[i]));
 		servaddr[i].sin_family = AF_INET;
		servaddr[i].sin_addr.s_addr= inet_addr(ip[i]);
 		servaddr[i].sin_port =  htons(port[i]); //convert to big-endian order

 		//Connection of the client to the socket 
 		if (connect(sockfd[i], (struct sockaddr *) &servaddr[i], sizeof(servaddr[i])) == -1) {
  			perror("Problem in connecting to the server");
  			flag = i;
  			close(sockfd[i]);
  			continue;
 		}
 	}

 	return 0;
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
 	struct timeval timeout;


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

		strcpy(key, password);
		printf("key is %s\n", key);
		keyLen = strlen(key);
		printf("keyLen is %d\n", keyLen);

 		//creating multiple sockets
 		if(create_sockets()){
 			printf("Sockets not created. Exiting...\n");
 			exit(1);
 		}


 		//menu like interface
		printf("\n-------------------------------------");
		printf("\nEnter one of these options:\n");
		printf("LIST\n");	
		printf("PUT [file_name]\n");	
		printf("GET [file_name]\n\n");	

		//getting input from user
		memset(options, 0, sizeof(options));
		fgets(options, 20, stdin);

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
			if(i==flag)
				continue;
			options_length = strlen(options);
			send(sockfd[i], &options_length, sizeof(options_length), 0);
			send(sockfd[i], options, options_length, 0);
		}				

		//sending out user credentials
		for(i=0; i<NO_OF_CONNECTIONS; i++){		
			if(i==flag)
				continue;			
			send(sockfd[i], &length_authenticate, sizeof(int), 0);
		}

		for(i=0; i<NO_OF_CONNECTIONS; i++){			
			if(i==flag)
				continue;		
			send(sockfd[i], concat_authenticate, length_authenticate, 0);
		}

		int temp_count = NO_OF_CONNECTIONS;

		//waiting for correct credentials reply from servers
		for(i=0; i<NO_OF_CONNECTIONS; i++){			
			if(i==flag)
				continue;		
			if((bytes_read[i] = recv(sockfd[i], &resp[i], sizeof(int), 0)) > 0){}	
			if(resp[i] == 1){
				//printf("Authentication successful with server %d\n", i);
				flag_authenticate = 0;
			}
			else{
				//printf("Invalid Username/Password. Please try again.\n");
				flag_authenticate = 1;
				temp_count--;
				//break;
			}
		}


		if(temp_count == active){
			printf("Authentication successful with servers\n");
		}
		else{
			printf("Invalid Username/Password. Please try again.\n");
			break;
		}

		switch(menu_id){
			//LIST feature
			case 0:
					printf("\n");

					timeout.tv_sec = 0;
					timeout.tv_usec = 1000000;	//setting a timeout of 1s

					//setting a timeout to receive ACK from server to 600ms
					/*for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;
						setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
					}	*/
					

					for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;
						folder_length = strlen(folder[i]);				
						//sending the length of the folder
						send(sockfd[i], &folder_length, sizeof(int), 0);
						//sending the folder name
						send(sockfd[i], folder[i], folder_length, 0);
					}


					//receiving the entire buffer of filenames and length
					for(i=0; i<NO_OF_CONNECTIONS; i++){
						if(i==flag)
							continue;
						memset(recv_list_buffer[i], 0, sizeof(recv_list_buffer[i]));
						recv(sockfd[i], &recv_list_size[i], sizeof(int), 0);
						recv(sockfd[i], recv_list_buffer[i], recv_list_size[i], 0);
					}

					printf("LIST OF SERVER FILES: -\n");
					printf("--------------------------------\n");
					printf("%s\n\n",recv_list_buffer[3]);	

					strcpy(recv_list_buffer_copy, recv_list_buffer[3]);


					char *str_list_temp;

					//creating a buffer of all the files called buffer_all_files
					str_list_temp = strtok(strdup(recv_list_buffer_copy), ".");
					memset(buffer_all_files, 0, sizeof(buffer_all_files));

					int count_line = 0;
					
					//extracting relevant data like only the filename from the buffer above
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
					count_line+=2;	//optimization

					/*printf("Buffer all files is :\n");
					printf("--------------------------\n");
					printf("%s\n", buffer_all_files);*/

					FILE *fp3 = fopen("temp.txt", "w");
					fwrite(buffer_all_files, sizeof(char), strlen(buffer_all_files), fp3);
					fclose(fp3);

					char buffer_files_copy[200];
					strcpy(buffer_files_copy, buffer_all_files);

					char line1[100];
					char *line_ptr1, *line_ptr2;
					char line_ptr_combined[100]; 
					char original_filenames[10][100];
					int count_original_files = 0;
					int ind = 0, count_loop = 0;
					char *status1, *temp1;

					fp3 = fopen("temp.txt", "r");
					memset(line1, 0, sizeof(line1));

					do {
						memset(line1, 0, sizeof(line1));
					    status1 = fgets(line1, sizeof(line1), fp3);
					    line_ptr1 = strtok_r(line1, ".", &temp1);
					   	line_ptr2 = strtok_r(NULL, ".", &temp1);
					   	sprintf(line_ptr_combined, "%s.%s", line_ptr1, line_ptr2);
					    //printf("line is %s\n", line_ptr_combined);
					    for(ind = 0; ind<count_original_files+1; ind++){
					    	//if matching, then break
					    	if(!strncmp(original_filenames[ind], line_ptr_combined, strlen(line_ptr_combined))){
					    		break;
					    	}
					    }
					    if(ind == count_original_files+1){
						    strcpy(original_filenames[count_original_files], line_ptr_combined);
						    ++count_original_files;
					    }
	
						memset(line_ptr_combined, 0, sizeof(line_ptr_combined));
						count_loop++;
					} while (status1 && count_loop<count_line);
					
					fclose(fp3);

					/*printf("LIST OF ORIGINAL FILES:\n");
					printf("----------------------------\n");
					for(ind=0; ind<count_original_files; ind++){
						printf("%s\n", original_filenames[ind]);
					}
					printf("\n");*/

					static int counts_arr[10];
					char line2[100];
					char temp_arr2[100];
					int n = 0;

					//iterating over the counts array
					for(n = 0; n < count_original_files; n++){
						memset(temp_arr2, 0, sizeof(temp_arr2));
						sprintf(temp_arr2, ".%s.0", original_filenames[n]);
						count_loop = 0;
						fp3 = fopen("temp.txt", "r");
						do{
							memset(line2, 0, sizeof(line2));
					    	status1 = fgets(line2, sizeof(line2), fp3);
							count_loop++;
							if(!strncmp(temp_arr2, line2, strlen(line2)-1)){
								counts_arr[n] += 1;
								break;
							}
						}while(status1 && count_loop<count_line);
						fclose(fp3);

						memset(temp_arr2, 0, sizeof(temp_arr2));
						sprintf(temp_arr2, ".%s.1", original_filenames[n]);
						count_loop = 0;
						fp3 = fopen("temp.txt", "r");
						do{
							memset(line2, 0, sizeof(line2));
					    	status1 = fgets(line2, sizeof(line2), fp3);
							count_loop++;
							if(!strncmp(temp_arr2, line2, strlen(line2)-1)){
								counts_arr[n] += 1;
								break;
							}
						}while(status1 && count_loop<count_line);
						fclose(fp3);

						memset(temp_arr2, 0, sizeof(temp_arr2));
						sprintf(temp_arr2, ".%s.2", original_filenames[n]);
						count_loop = 0;
						fp3 = fopen("temp.txt", "r");
						do{
							memset(line2, 0, sizeof(line2));
					    	status1 = fgets(line2, sizeof(line2), fp3);
							count_loop++;
							if(!strncmp(temp_arr2, line2, strlen(line2)-1)){
								counts_arr[n] += 1;
								break;
							}
						}while(status1 && count_loop<count_line);
						fclose(fp3);

						memset(temp_arr2, 0, sizeof(temp_arr2));
						sprintf(temp_arr2, ".%s.3", original_filenames[n]);
						count_loop = 0;
						fp3 = fopen("temp.txt", "r");
						do{
							memset(line2, 0, sizeof(line2));
					    	status1 = fgets(line2, sizeof(line2), fp3);
							count_loop++;
							if(!strncmp(temp_arr2, line2, strlen(line2)-1)){
								counts_arr[n] += 1;
								break;
							}
						}while(status1 && count_loop<count_line);
						fclose(fp3);												
					}					


					//final decision on files being complete or incomplete
					printf("STATUS OF FILES ON SERVER:\n");
					printf("---------------------------\n");

					int r = 0;
					for(r=0; r<count_original_files; r++){
						if(counts_arr[r] >= NO_OF_CONNECTIONS)
							printf("%s [complete]\n", original_filenames[r]);
						else
							printf("%s [incomplete]\n", original_filenames[r]);
					}

					printf("\n\n");
					timeout.tv_sec = 0;
					timeout.tv_usec = 0;	//setting a timeout of 0s

					/*for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;
						setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
					}*/	


					break;
			//PUT functionality
			case 1:
					printf("\n");
					printf("PUT option - check\n");

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

					//calculating md5sum of file content and performing modulus of 4
					x = md5sum_mod(fp);
					printf("x is %d\n", x);
					printf("Length of %s is %d\n", filename, length_of_file);
					fclose(fp);

					int sum = 0;

					timeout.tv_sec = 0;
					timeout.tv_usec = 1000000;	//setting a timeout of 1s

					//setting a timeout to receive ACK from server to 600ms
					/*for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;
						setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
					}*/	

					//setting the lengths of part files
					for(i = 0; i < NO_OF_CONNECTIONS; i++){
						if(i==flag)
							continue;
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

					int loop_cnt = 0;

					for(i = 0; i < NO_OF_CONNECTIONS; i++){
						//sending out the value of x
						if(i==flag)
							continue;					
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

							//encrypting data part before sending it out to the server
							#ifdef ENCRYPTION
							dataLen = length_part_file[i];
							loop_cnt = 0;
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

							send(sockfd[i], buffer_part[i], length_part_file[i] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+1)%4]);

							#ifdef ENCRYPTION							
							dataLen = length_part_file[(i+1)%4];
							loop_cnt = 0;
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

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


							#ifdef ENCRYPTION
							dataLen = length_part_file[(i+3)%4];
							loop_cnt = 0;
							
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

							send(sockfd[i], buffer_part[i], length_part_file[(i+3)%4] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[i]);

							#ifdef ENCRYPTION
							dataLen = length_part_file[i];
							loop_cnt = 0;
							
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

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

							#ifdef ENCRYPTION
							dataLen = length_part_file[(i+2)%4];
							loop_cnt = 0;
							
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

							send(sockfd[i], buffer_part[i], length_part_file[(i+2)%4] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+3)%4]);

							#ifdef ENCRYPTION
							dataLen = length_part_file[(i+3)%4];
							loop_cnt = 0;
							
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

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

							#ifdef ENCRYPTION							
							dataLen = length_part_file[(i+1)%4];
							loop_cnt = 0;
							
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

							send(sockfd[i], buffer_part[i], length_part_file[(i+1)%4] + 1, 0);

							//sending the part file
							memset(buffer_part[i], 0, sizeof(buffer_part[i]));
							strcpy(buffer_part[i], storage_buf[(i+2)%4]);

							#ifdef ENCRYPTION
							dataLen = length_part_file[(i+2)%4];
							loop_cnt = 0;
							
							//xor encryption using password
							while(loop_cnt<dataLen){
    							buffer_part[i][loop_cnt] ^= key[loop_cnt % (keyLen-1)]; 
    							++loop_cnt;
    							//flag=1;
	   						}
	   						#endif

							send(sockfd[i], buffer_part[i], length_part_file[(i+2)%4] + 1, 0);	
										
						}

						timeout.tv_sec = 0;
						timeout.tv_usec = 0;	//setting a timeout of 0s

						//setting a timeout to receive ACK from server to 600ms
						/*for(i=0; i<NO_OF_CONNECTIONS;i++){

							setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
						}*/

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

					timeout.tv_sec = 0;
					timeout.tv_usec = 1000000;	//setting a timeout of 1s

					/*
					//setting a timeout to receive ACK from server to 600ms
					for(i=0; i<NO_OF_CONNECTIONS;i++){
						setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
					}*/


					for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;
						folder_length = strlen(folder[i]);				
						//sending the length of the folder
						send(sockfd[i], &folder_length, sizeof(int), 0);
						//sending the folder name
						send(sockfd[i], folder[i], folder_length, 0);
					}

					//receiving the number of unique files from each server
					for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;						
						recv(sockfd[i], &recv_file_count[i], sizeof(int), 0);
						printf("File count for server %d is %d\n", i, recv_file_count[i] );
					}

					for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;					
						//receive the length of filename
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
								//check if the previous copy was good and had printable characters
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
					}

					char *name;
					char name_arr[20], temp_sprintf[20], rec_name0[20], rec_name1[20], rec_name2[20], rec_name3[20];
					int len_0, len_1, len_2, len_3;
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
					//recombining the file from individual parts
					char temp_parts[BUFFER_SIZE];
					printf("x is %d\n", x);

					if( (access( rec_name0, F_OK ) != -1 ) && \
					(access( rec_name1, F_OK ) != -1 ) && (access( rec_name2, F_OK ) != -1 ) \
					&& (access( rec_name3, F_OK ) != -1 )){ 
						//file exists
						memset(combined_file, 0, sizeof(combined_file));
						printf("Complete file exists.\n");
						fp = fopen(rec_name1, "r");
						memset(temp_parts, 0, sizeof(temp_parts));
						len_2 = file_length(fp);								
						len_2 = fread(temp_parts, sizeof(char), len_2, fp );
						static int d = 0;
						while(temp_parts[d]=='\0')
							d++;
						//printf("1 is: %s and len2 is %d\n", temp_parts + i, len_2);								
						strcat(combined_file, temp_parts + d);
						fclose(fp);
						fp = fopen(rec_name2, "r");
						memset(temp_parts, 0, sizeof(temp_parts));
						len_3 = file_length(fp);								
						len_3 = fread(temp_parts, sizeof(char), len_3 , fp );
						d = 0;
						while(temp_parts[d]=='\0')
							d++;
						//printf("2 is: %s and len3 is %d\n", temp_parts + i, len_3);
						strcat(combined_file, temp_parts + d);
						fclose(fp);
						fp = fopen(rec_name3, "r");
						memset(temp_parts, 0, sizeof(temp_parts));
						len_0 = file_length(fp);
						len_0 = fread(temp_parts, sizeof(char), len_0, fp );
						d = 0;
						while(temp_parts[d]=='\0')
							d++;
						//printf("3 is: %s and len0 is %d\n", temp_parts + i, len_0);
						strcat(combined_file, temp_parts + d);
						fclose(fp);
						fp = fopen(rec_name0, "r");
						memset(temp_parts, 0, sizeof(temp_parts));
						len_1 = file_length(fp);
						len_1 = fread(temp_parts, sizeof(char), len_1, fp );
						d = 0;
						while(temp_parts[d]=='\0')
							d++;
						//printf("0 is: %s and len1 is %d\n", temp_parts + i, len_1);
						strcat(combined_file, temp_parts + d);
						fclose(fp);
						strcat(combined_file, "\0");

						//combined file
						char received_file[20];
						sprintf(received_file, "received_%s", name_arr);

						//writing to a file
						fp = fopen(received_file, "w");
						fwrite(combined_file, sizeof(char), strlen(combined_file), fp);
						fclose(fp);
					}
					else{
						printf("Incomplete.\n");
					}	

					char clean_up_arr[40];
					sprintf(clean_up_arr, "rm %s %s %s %s", rec_name0, rec_name1, rec_name2, rec_name3);						
					system(clean_up_arr);

					timeout.tv_sec = 0;
					timeout.tv_usec = 0;	//setting a timeout of 0s

					/*for(i=0; i<NO_OF_CONNECTIONS;i++){
						if(i==flag)
							continue;						
						setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
					}	*/


					break;
			//default
			default:
					printf("\n");		
					printf("Enter option correctly.\n");		
		}

		//closing the socket
		int y = 0;
		for(y=0; y< NO_OF_CONNECTIONS; y++){
			if(y==flag)
				continue;			
			close(sockfd[y]);
			sockfd[y] = -1;
		}

	}


	return 0;
}