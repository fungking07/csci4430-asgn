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
#include <errno.h>
#include "myftp.h"
#include <pthread.h>
pthread_mutex_t mutex;
//to list the file on server
void list(int fd)
{
	int len_s,len_r;
	char* buff;
	struct message_s request;
	struct message_s reply;
	set_protocol(&request,0xA1,HEADER_LENGTH);
	if((len_s=send(fd,(void*)&request,sizeof(request),0))==-1)
	{
		printf("Fail on LIST_REQUEST_PROTOCOL\n");
		close(fd);
		exit(0);
	}
	buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
	memset(buff,'\0',sizeof(char)*BUFF_SIZE);
	if((len_r=recv(fd,buff,BUFF_SIZE,0))==-1)
	{
		printf("Faile on LIST_REPLY_PROTOCOL\n");
		close(fd);
		exit(0);
	}
	memcpy(&reply,buff,HEADER_LENGTH);
	if(type_to_int(reply,len_r)!=LIST_REPLY)
	{
		printf("Wrong protocol type\n");
		close(fd);
		exit(0);
	}
	printf("%s",&buff[HEADER_LENGTH]);
	free(buff);
}

//to download file from server
void download(int fd,char* path)
{
	int len_s,len_r,len_d,data_len;
	unsigned int filesize;
	unsigned int payload;
	char* buff;
	char* file_buff;
	FILE *fp;
	struct message_s request;
	struct message_s reply;
	struct message_s data;
	payload=strlen(path)+1;
	set_protocol(&request,0xB1,HEADER_LENGTH+payload);
	buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
	memset(buff,'\0',sizeof(char)*BUFF_SIZE);
	strcpy(&buff[HEADER_LENGTH],path);
	memcpy(buff,&request,HEADER_LENGTH);
	if((len_s=send(fd,buff,HEADER_LENGTH+payload,0))==-1)
	{
		printf("Fail on GET_REQUEST_PROTOCOL\n");
		close(fd);
		exit(0);
	}
	free(buff);
	if((len_r=recv(fd,&reply,HEADER_LENGTH,0))==-1)
	{
		printf("Fail on GET_REPLY_PROTOCOL\n");
	}
	if(type_to_int(reply,len_r)==GET_REPLY_EXISTS)
	{
		if((len_d=recv(fd,&data,sizeof(data),0))==-1)
		{
			printf("Fail on FILE_DATA_PROTOCOL\n");
			close(fd);
			exit(0);
		}
	}
	else if(type_to_int(reply,len_r)==GET_REPLY_NOT_EXISTS)
	{
		printf("Sucess on GET_REPLY_PROTOCOL but FILE DOES NOT EXITS\n");
		close(fd);
		exit(0);
	}
	else
	{
		printf("Wrong protocol type\n");
		close(fd);
		exit(0);
	}
	filesize=ntohl(data.length)-HEADER_LENGTH;
	file_buff=(char*)malloc(sizeof(char)*MAX_SIZE);
	fp=fopen(path,"wb");
	if(fp==NULL)
	{
		printf("Fail on opening file\n");
		close(fd);
		exit(0);
	}
	if((data_len=recvn(fd,file_buff,filesize))==-1)
	{
		printf("Fail on receiving file\n");
		close(fd);
		fclose(fp);
		exit(0);
	}
	fwrite(file_buff,1,data_len,fp);
	fclose(fp);
}

//to upload file to server
void upload(int fd,char* filename)
{
	int len_s,len_r,len_d;
	unsigned int payload;
	unsigned int file_payload;
	char* buff;
	char* file_data;
	FILE* fp;
	size_t file_bytes;
	struct message_s request;
	struct message_s reply;
	struct message_s data;
	if(access(filename,0)<0)
	{
		printf("NO EXISTING FILE\n");
		close(fd);
		exit(0);
	}
	payload=strlen(filename)+1;
	set_protocol(&request,0xC1,HEADER_LENGTH+payload);
	buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
	memset(buff,'\0',sizeof(char)*BUFF_SIZE);
	strcpy(&buff[HEADER_LENGTH],filename);
	memcpy(buff,&request,HEADER_LENGTH);
	if((len_s=send(fd,buff,HEADER_LENGTH+payload,0))==-1)
	{
		printf("Fail on PUT_REQUEST_PROTOCOL\n");
		close(fd);
		free(buff);
		exit(0);
	}
	free(buff);
	if((len_r=recv(fd,&reply,HEADER_LENGTH,0))==-1)
	{
		printf("Fail on PUT_REPLY_PROTOCOL\n");
		close(fd);
		exit(0);
	}	
	file_payload=0;
	fp=fopen(filename,"r");
	if(fp==NULL)
	{
		printf("Fail on openning file\n");
		close(fd);
		exit(0);
	}
	fseek(fp,0,SEEK_END);
	file_payload += ftell(fp);
	fseek(fp,0,SEEK_SET);
	fclose(fp);
	set_protocol(&data,0xFF,HEADER_LENGTH+file_payload);
	if((len_s=send(fd,(void*)&data,HEADER_LENGTH,0))==-1)
	{
		printf("Fail on FILE_DATA_PROTOCOL\n");
		close(fd);
		exit(0);
	}
	file_bytes = 0;
	file_data=(char*)malloc(sizeof(char)*MAX_SIZE);
	fp=fopen(filename,"r");
	if(fp==NULL)
	{
		printf("Fail on openning file\n");
		close(fd);
		exit(0);
	}
	int byte_left=file_payload;
	while((file_bytes=fread(file_data,1,CHUNK_SIZE,fp))>0)
	{
		if((len_d=sendn(fd,file_data,file_bytes))==-1)
		{
			printf("Fail on sending file\n");
			close(fd);
			exit(0);
		}
		byte_left -= file_bytes;
		//printf("bytes left: %d\n",byte_left);
	}
}
//this function use pthread to simulate 10 users access to server in the same time
void *test(void* args)
{
	int *thread_num_p = (int *)args;
  	int thread_num = *thread_num_p;
	//pthread_mutex_lock(&mutex);
	printf("this is thread %d\n",thread_num);
	int fd;
	unsigned int addrlen=sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	in_addr_t ip=inet_addr("192.168.1.107");
	unsigned short port=atoi("12345");
	fd=socket(AF_INET, SOCK_STREAM, 0);
	if(fd==-1)
	{
		perror("socket()");
		exit(1);
	}
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr =ip;
	addr.sin_port=htons(port);
	if(connect(fd,(struct sockaddr *)&addr,addrlen)==-1)
	{
		perror("connect()");
		close(fd);
		exit(0);
	}
	printf("now call list()\n");
	list(fd);
	printf("finished thread %d\n",thread_num);
	//pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
	return 0;
}
//main
int main(int argc,char **argv)
{
	int flag=-1;
	if(argc<4)
	{
		printf("Invalid command!\n");
		exit(0);
	}
	else if(argc==4)
	{
		if(strcmp(argv[3],"list")==0)
		{
			flag=LIST_REQUEST;
		}
		else
		{
			printf("Invalid command!\n");
			exit(0);
		}
		
	}
	else if(argc==5)
	{
		if(strcmp(argv[3],"put")==0)
		{
			flag=PUT_REQUEST;
		}
		else if(strcmp(argv[3],"get")==0)
		{
			flag=GET_REQUEST;
		}
		else
		{
			printf("Invalid command!\n");
			exit(0);
		}
	}
	else
	{
		printf("Invalid command!\n");
		exit(0);
	}
	//use for testing multi-threading in server
	/*pthread_t thread[10];
	int arg[10];
  	for (int i = 0; i < 10; i++) {
    arg[i] = i + 1;
  	}
	for(int i=0;i<10;i++)
	{
		int val=pthread_create(&thread[i],NULL,test,&arg[i]);
	}
	for(int i=0;i<10;i++)
	{
		pthread_join(thread[i],NULL);
	}*/
	int fd;
	unsigned int addrlen=sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	in_addr_t ip=inet_addr(argv[1]);
	unsigned short port=atoi(argv[2]);
	fd=socket(AF_INET, SOCK_STREAM, 0);
	if(fd==-1)
	{
		perror("socket()");
		exit(1);
	}
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr =ip;
	addr.sin_port=htons(port);
	if(connect(fd,(struct sockaddr *)&addr,addrlen)==-1)
	{
		perror("connect()");
		close(fd);
		exit(0);
	}
	switch(flag)
	{
		case LIST_REQUEST:
			list(fd);
			break;
		case GET_REQUEST:
			download(fd,argv[4]);
			break;
		case PUT_REQUEST:
			upload(fd,argv[4]);
			break;
		default:
			printf("unknown command!\n");
			break;
	}
	close(fd);
	
	return 0;
}
