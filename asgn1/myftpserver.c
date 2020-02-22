#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	// "struct sockaddr_in"
#include <arpa/inet.h>	// "in_addr_t"
#include <dirent.h>  // struct dirent *readdir(DIR *dirp);
#include <errno.h>
#include "myftp.h"

int accept_client(int fd, struct sockaddr *__restrict__ addr){
    int addr_len = sizeof(addr);
    int client_sd;
    if((client_sd = accept(fd,(struct sockaddr *) &addr, &addr_len)) < 0){
        printf("accept error: %s (Errno:%d)\n",strerror(errno),errno);
        exit(0);
    }
}

int receive_msg(int sd, char** data){
    char* buff = (char*)malloc(sizeof(char)*BUFF_SIZE);
    memset(buff, '\0', sizeof(char)*BUFF_SIZE);
    struct message_s msg;
    int len;
    if((len = recv(sd, buff, BUFF_SIZE, 0)) < 0){
        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
    }
    memcpy(&msg, buff, HEADER_LENGTH);
    printf("name size %d\n", ntohl(msg.length) - HEADER_LENGTH);
    printf("protocol: %s type: %d len: %d\n", msg.protocol, msg.type, ntohl(msg.length));
    int action = type_to_int(msg, len);
    printf("%d\n", action);
    if(action == -1){
        printf("protocol error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(-1);
    }
    if(action != LIST_REQUEST){
        int filename_length = ntohl(msg.length) - HEADER_LENGTH;
        printf("===%d\n", filename_length);
        *data = (char*)malloc(sizeof(char) * filename_length);
        memcpy(*data, buff + HEADER_LENGTH, sizeof(buff));
        //printf("%s", *data);
    }
    return action;
}

int list(int fd){
    return 1;
}
int send_file(int fd, char* file){
    return 1;
}
int receive_file(int fd, char* file){
    return 1;
}

int main(int argc, char** argv){
    if(argc != 2){
        printf("Invalid command!\n");
    }
	int sd=socket(AF_INET,SOCK_STREAM,0);
    char* buff;
	int client_sd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	server_addr.sin_port=htons(atoi(argv[1]));
	if(bind(sd, (struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	if(listen(sd, 10)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
    while(1){
        client_sd = accept_client(sd, (struct sockaddr *) &client_addr);
        buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
        memset(buff,'\0',sizeof(char)*BUFF_SIZE);
        int code = receive_msg(client_sd, &buff);
        printf("%s\n", buff);
        switch(code){
            case LIST_REQUEST:
                printf("list\n");
                list(client_sd);
                break;
            case GET_REQUEST:
                printf("get file\n");
                send_file(client_sd, buff);
                break;
            case PUT_REQUEST:
                printf("put file\n");
                receive_file(client_sd, buff);
                break;
            default:
                printf("receive code error: %s (Errno:%d)\n", strerror(errno), errno);
                exit(-1);
        }
        free(buff);
        close(client_sd);
    }
    return 0;
}