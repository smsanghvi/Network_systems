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

#define MAX_CONF_SIZE 1000

static char folder_1[10], folder_2[10];
static char folder_3[10], folder_4[10];
static char ip1[20], ip2[20], ip3[20], ip4[20];
int port1, port2, port3, port4;
char username[20];
char password[20];
char *str;
int count = 0;

char buf_conf[MAX_CONF_SIZE];




int main(){

	return 0;
}