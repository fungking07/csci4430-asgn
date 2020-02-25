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
#include <pthread.h>
#include <errno.h>
#include "myftp.h"

#define PATH "data/"

int accept_client(int fd, struct sockaddr *__restrict__ addr){
    int addr_len = sizeof(addr);
    int client_sd;
    if((client_sd = accept(fd,(struct sockaddr *) &addr, &addr_len)) < 0){
        printf("accept error: %s (Errno:%d)\n",strerror(errno),errno);
        exit(0);
    }
    printf("client fd: %d\n", client_sd);
    printf("fd: %d\n", fd);
    return client_sd;
}

int receive_msg(int sd, char** data){
    char* buff = (char*)malloc(BUFF_SIZE);
    memset(buff, '\0', BUFF_SIZE);
    struct message_s msg;
    int len;
    if((len = recv(sd, buff, BUFF_SIZE, 0)) == -1){
        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
    }
    if(len == 0){
        printf("no\n");
        return PUT_FILE_NOT_EXIST;
    }
    memcpy(&msg, buff, HEADER_LENGTH);
    int action = type_to_int(msg, len);
    if(action == -1){
        printf("protocol error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
    }
    if(action != LIST_REQUEST){
        int filename_length = ntohl(msg.length) - HEADER_LENGTH;
        *data = (char*)malloc(sizeof(char) * filename_length);
        memset(*data, '\0', sizeof(char) * filename_length);
        memcpy(*data, buff + HEADER_LENGTH, filename_length);
    }
    free(buff);
    return action;
}

void list(int fd){
    char* buff = (char*)malloc(BUFF_SIZE);
    DIR* curr_dir = opendir(PATH);
    if(curr_dir == NULL){
        printf("cannot open directory ./data/\n");
        exit(0);
    }
    // struct dirent {
    //     ino_t          d_ino;       /* Inode number */
    //     off_t          d_off;       /* Not an offset; see below */
    //     unsigned short d_reclen;    /* Length of this record */
    //     unsigned char  d_type;      /* Type of file; not supported
    //                                     by all filesystem types */
    //     char           d_name[256]; /* Null-terminated filename */
    // };
    struct message_s msg;
    int payload = 0;
    struct dirent* folder = NULL;
    while((folder = readdir(curr_dir)) != NULL){
        if(strcmp(folder->d_name, ".") != 0 && strcmp(folder->d_name, "..") != 0){
            strcpy(&buff[HEADER_LENGTH + payload], folder->d_name);
            payload += strlen(folder->d_name)+1;
            strcpy(&buff[HEADER_LENGTH + payload - 1], "\n");
        }
    }
    closedir(curr_dir);
    set_protocol(&msg, 0xA2, HEADER_LENGTH + payload);
    memcpy(buff, &msg, HEADER_LENGTH);
    if(send(fd, buff, HEADER_LENGTH + payload, 0) == -1){
        printf("list error\n");
        exit(0);
    }
    free(buff);
}
void send_file(int fd, char* file){
    char *file_path = (char*)malloc(strlen(PATH) + strlen(file) + 1);
    memset(file_path, '\0', strlen(PATH) + strlen(file) + 1);
    memcpy(file_path, &PATH, sizeof(PATH));
    memcpy(file_path + strlen(PATH), file, strlen(file));
    char *buff = (char*)malloc(BUFF_SIZE);
    memset(buff, '\0', sizeof(buff));
    struct message_s msg;
    int len = 0;
    FILE *fp = fopen(file_path, "r");
    if(fp == NULL){
        printf("cannot open requested file\n");
        set_protocol(&msg, 0xB3, HEADER_LENGTH);
        memcpy(buff, &msg, HEADER_LENGTH);
        if(send(fd, buff, HEADER_LENGTH, 0) == -1){
            printf("sent GET_REPLY_NOT_EXITST protocol error\n");
            exit(0);
        }
        free(buff);
        return;
    }
    else{
        // read file size
        fseek(fp, 0, SEEK_END);
        long int total_file_size = ftell(fp);
        printf("%s\n", file_path);
        fseek(fp, 0, SEEK_SET); // return to top
        
        set_protocol(&msg, 0xB2, HEADER_LENGTH);
        memset(buff, '\0', sizeof(buff));
        memcpy(buff, &msg, HEADER_LENGTH);
        if(send(fd, buff, HEADER_LENGTH, 0) == -1){
            printf("sent GET_REPLY_EXITST protocol error\n");
            exit(0);
        }

        set_protocol(&msg, 0xFF, HEADER_LENGTH + total_file_size);
        //memset(buff, '\0', sizeof(buff));
        //memcpy(buff, &msg, HEADER_LENGTH);
        if((len=send(fd, (void*)&msg, HEADER_LENGTH, 0)) == -1){
            printf("sent FILE_DATA protocol error\n");
            exit(0);
        }
        char *file_data = (char*)malloc(sizeof(char)*MAX_SIZE);
        int read_file_size = 0;
        int remaining_byte=total_file_size;
        while((read_file_size = fread(file_data, 1, CHUNK_SIZE, fp)) > 0){
            if((len=sendn(fd, file_data, read_file_size)) == -1){
                fclose(fp);
                close(fd);
                printf("send file error\n");
                exit(0);
            }
            remaining_byte-=read_file_size;
            printf("remaining %d bytes\n",remaining_byte);
        }
        printf("sent\n");
        free(file_data);
    }

    fclose(fp);
    free(buff);
    free(file_path);
}
void receive_file(int fd, char* file){
    char *buff = (char*)malloc(BUFF_SIZE);
    memset(buff, '\0', sizeof(buff));
    struct message_s msg, msg_file;
    set_protocol(&msg, 0xC2, HEADER_LENGTH);
    memcpy(buff, &msg, HEADER_LENGTH);
    if(send(fd, buff, HEADER_LENGTH, 0) < 0){
        printf("cannot sent put reply\n");
        exit(0);
    }

    memset(buff, '\0', sizeof(buff));
    if(recv(fd, buff, HEADER_LENGTH, 0) < 0){
        printf("file protocol error\n");
        exit(0);
    }
    memcpy(&msg_file, buff, HEADER_LENGTH);
    int len = ntohl(msg_file.length) - HEADER_LENGTH;

    char *file_path = (char*)malloc(strlen(PATH) + strlen(file) + 1);
    memset(file_path, '\0', strlen(PATH) + strlen(file) + 1);
    memcpy(file_path, &PATH, sizeof(PATH));
    memcpy(file_path + strlen(PATH), file, strlen(file));

    FILE *fp = fopen(file_path, "wb");
    if(fp==NULL)
    {
        printf("Fail to create file\n");
        close(fd);
        exit(0);
    }
    char *file_data = (char*)malloc(MAX_SIZE*sizeof(char));
    //memset(file_data, '\0', MAX_SIZE*sizeof(char));
    int data_len;
    if(type_to_int(msg_file, HEADER_LENGTH + len) == FILE_DATA){
        if((data_len = recvn(fd, file_data, len)) >= 0){
            fwrite(file_data, 1, data_len, fp);
        }
        else{
            printf("receive data error\n");
            exit(0);
        }
    }
    else{
        printf("%d\n", type_to_int(msg_file, HEADER_LENGTH + len));
        printf("file data protocol error\n");
        exit(0);
    }
    fclose(fp);
    free(file_data);
    free(file_path);
    free(buff);
}

int main(int argc, char** argv){
    if(argc != 2){
        printf("Invalid command!\n");
        exit(0);
    }
	int sd=socket(AF_INET,SOCK_STREAM,0);
    char* buff;
	int client_sd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(atoi(argv[1]));
	if(bind(sd, (struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	if(listen(sd, 10)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
    printf("I am listening\n");
    while(1){
        client_sd = accept_client(sd, (struct sockaddr *) &client_addr);
        buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
        memset(buff,'\0',sizeof(char)*BUFF_SIZE);
        int code = receive_msg(client_sd, &buff);
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
            case PUT_FILE_NOT_EXIST:
                printf("nothing to do\n");
                break;
            default:
                printf("receive code error: %s (Errno:%d)\n", strerror(errno), errno);
                exit(0);
        }
        free(buff);
        close(client_sd);
    }
    return 0;
}
