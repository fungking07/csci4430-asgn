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
#include <isa-l.h>
//to list the file on server
void list(int fd[],int standby[],int n)
{
	int id=0;
	//choose a connected server fd
	for(;id<n;id++)
	{
		if(standby[id]==1)
			break;
	}
	int len_s,len_r;
	char* buff;
	struct message_s request;
	struct message_s reply;
	set_protocol(&request,0xA1,HEADER_LENGTH);
	if((len_s=send(fd[id],(void*)&request,sizeof(request),0))==-1)
	{
		printf("Fail on LIST_REQUEST_PROTOCOL\n");
		for(int i=0;i<n;i++)
		{
			if(standby[i]==1)
				close(fd[i]);
		}
		exit(0);
	}
	buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
	memset(buff,'\0',sizeof(char)*BUFF_SIZE);
	if((len_r=recv(fd[id],buff,BUFF_SIZE,0))==-1)
	{
		printf("Faile on LIST_REPLY_PROTOCOL\n");
		for(int i=0;i<n;i++)
		{
			if(standby[i]==1)
				close(fd[i]);
		}
		exit(0);
	}
	memcpy(&reply,buff,HEADER_LENGTH);
	if(type_to_int(reply,len_r)!=LIST_REPLY)
	{
		printf("Wrong protocol type\n");
		for(int i=0;i<n;i++)
		{
			if(standby[i]==1)
				close(fd[i]);
		}
		exit(0);
	}
	printf("%s",&buff[HEADER_LENGTH]);
	free(buff);
}

//to download file from server
void download(char* path,int fd[],int n,int k)
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
void upload(char* filename,int fd[],int n,int k,int block_size)
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
//main
int main(int argc,char **argv)
{
	int flag=-1;
	if(argc<3)
	{
		printf("Invalid command!\n");
		exit(0);
	}
	else if(argc==3)
	{
		if(strcmp(argv[2],"list")==0)
		{
			flag=LIST_REQUEST;
		}
		else
		{
			printf("Invalid command!\n");
			exit(0);
		}
	}
	else if(argc==4)
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
	FILE *client_config;
	client_config=fopen(argv[1],"r");
	if(client_config == NULL)
	{
		printf("client config file NOT EXIST!\n");
		exit(0);
	}
	int n,k,block_size;
	fscanf(client_config,"%d %d %d",&n,&k,&block_size);
    fgetc(client_config);
	printf("client config info:\n");
    printf("n= %d\nk= %d\nblock_size= %d\n",n,k,block_size);
	if(block_size>CHUNK_SIZE)
	{
		printf("block_size is out of size!\n");
		exit(0);
	}
	char ip_addr[n][20],ports[n][10],c;
	int counta=0,countb=0,countc=0,control_flag=0;
	//get info from clientconfig.txt
	while(control_flag != 3)
	{
		c=fgetc(client_config);
		if(c == ':')
		{
			control_flag=1;
			continue;
		}
		if(c == '\n')
			control_flag=2;
		if(c == EOF)
			control_flag=3;
		if(control_flag == 0)
		{
			ip_addr[countc][counta]=c;
			counta++;
		}
		if(control_flag == 1)
		{
			ports[countc][countb]=c;
			countb++;
		}
		if(control_flag == 2 || control_flag == 3)
		{
			ip_addr[countc][counta]='\0';
			ports[countc][countb]='\0';
			if(control_flag !=3)
				control_flag=0;
			counta=0;
			countb=0;
			countc++;
		}
	}
    for(int i =0;i<n;i++)
    {
        printf("server ip %d : %s:%s\n",i+1,ip_addr[i],ports[i]);		//print info
    }
	//build connection to servers
	int fd[n],standby[n];			//standby[n] is recorded wheter the server is connected successful,0 is fail, 1 is success
	int all_on=1;
	for(int i = 0; i < n ; i ++)
	{
		int addrlen=sizeof(struct sockaddr_in);
		struct sockaddr_in addr;
		in_addr_t ip=inet_addr(ip_addr[i]);
		unsigned short port=atoi(ports[i);
		fd[i]=socket(AF_INET, SOCK_STREAM, 0);
		if(fd[i]==-1)
		{
			perror("socket()");
			exit(0);
		}
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr =ip;
		addr.sin_port=htons(port);
		if(connect(fd[i],(struct sockaddr *)&addr,addrlen)==-1)
		{
			perror("connect()");
			close(fd[id]);
			standby[i]=0;
			all_on=0;
		}
		else
			standby[i]=1;
	}
	switch(flag)
	{
		case LIST_REQUEST:
			list(fd,n);
			break;
		case GET_REQUEST:
			download(argv[4],fd,n,k);
			break;
		case PUT_REQUEST:
			if(all_on)
				upload(argv[4],fd,n,k,block_size);
			else
				printf("Servers are not all connected!!\n");
			break;
		default:
			printf("unknown command!\n");
			break;
	}
	
	return 0;
}
