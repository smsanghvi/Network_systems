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
#include <openssl/md5.h>

#define MAX_CONF_SIZE 1000
#define BUFFER_SIZE 1000000
#define NO_OF_CONNECTIONS 4

static char folder[NO_OF_CONNECTIONS][10];
static char ip[NO_OF_CONNECTIONS][20];
int port[NO_OF_CONNECTIONS];
char username[20];
char password[20];
char options[20];
char *str;
int16_t menu_id = -1;	
int count = 0, x = 0;
int sockfd[NO_OF_CONNECTIONS];
struct sockaddr_in servaddr[NO_OF_CONNECTIONS];
char storage_buf[NO_OF_CONNECTIONS][BUFFER_SIZE];
int options_length = 0;


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
    md5_bytes = fread(md5_buf, 512, 1, fp);
    while(md5_bytes > 0)
    {
            MD5_Update(&md5_c, md5_buf, md5_bytes);
            md5_bytes = fread(md5_buf, 512, 1, fp);
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


int main(int argc, char **argv){
	int i;
	char buffer_part[NO_OF_CONNECTIONS][BUFFER_SIZE];
	int bytes_read[4];
 	int resp[4];
 	int length_authenticate;
 	char concat_authenticate[40];
 	char *temp_str;
 	char filename[20];
 	int length_of_file;
 	int length_part_file[NO_OF_CONNECTIONS];
 	int flag_authenticate = 0;

	//basic check for the arguments
 	if (argc != 2) {
  		printf("Usage: dfclient <conf file>\n"); 
  		exit(1);
 	}

 	//opening the conf file
	FILE *fp = fopen(argv[1], "r");
	printf("Parsing the client configuration file...\n");
	parse_conf(fp);
	fclose(fp);

	strcpy(concat_authenticate, "\n");
	strcat(concat_authenticate, username);
	strcat(concat_authenticate, " ");
	strcat(concat_authenticate, password);
	length_authenticate = strlen(concat_authenticate);
	printf("Concat is %s\n", concat_authenticate);
	printf("Strlen is %d\n", length_authenticate);

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
	fgets(options, 20, stdin);

	while(1){
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
					int folder_length;

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

					break;
			//default
			case 3:
					printf("\n");
		
					printf("Enter option correctly.\n");		
		}

		break;
	}


	return 0;
}