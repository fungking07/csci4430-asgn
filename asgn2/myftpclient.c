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
//global variables stored in client config
int n,k,block_size;

void close_all_connection(int* fd, int* standby)
{
	for(int i = 0; i < n; i++){
		if(standby[i] == 1) close(fd[i]);
	}
}


//to list the file on server
void list(int* fd, int* standby)
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
		close_all_connection(fd, standby);
		exit(0);
	}
	buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
	memset(buff,'\0',sizeof(char)*BUFF_SIZE);
	if((len_r=recv(fd[id],buff,BUFF_SIZE,0))==-1)
	{
		printf("Faile on LIST_REPLY_PROTOCOL\n");
		close_all_connection(fd, standby);
		exit(0);
	}
	memcpy(&reply,buff,HEADER_LENGTH);
	if(type_to_int(reply,len_r)!=LIST_REPLY)
	{
		printf("Wrong protocol type\n");
		close_all_connection(fd, standby);
		exit(0);
	}
	printf("%s",&buff[HEADER_LENGTH]);
	free(buff);
}

//to download file from server
void download(char* path, int* list_fd, int fd, int* standby)
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

	// exit if not enough available server
	int num_available_server = 0;
	for(int i = 0; i < n; i ++){
		if(standby[i] == 1) num_available_server++;
	}
	if(num_available_server < k){
		printf("Only %d server available, need %d server to download\n", num_available_server, k);
		close_all_connection(list_fd, standby);
		exit(0);
	}

	// send and receive header -> download or cancel
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

	printf("start decoding file\n");

	// start decoding
	uint8_t *decode_matrix = malloc(sizeof(uint8_t) * k * k);
	uint8_t *matrix = malloc(sizeof(uint8_t) * n * k);
	uint8_t *invert_matrix = malloc(sizeof(uint8_t) * k * k);
	uint8_t *tables = malloc(sizeof(uint8_t) * (32 * (n-k) * k));
	gf_gen_rs_matrix(matrix, n, k);

	// copy a set of valid server to decode_matrix
	int row = 0;
	for(int i = 0; i < n; i++){
		if(standby[row] == 0){
			continue;
		}
		for(int j = 0; j < k; j++){
			decode_matrix[(row * k) + j] = matrix[(i * k) * j];
		}
		row++;
	}
	// print decode matrix
	for(int i = 0; i < k; i++)
	{
		for(int j = 0; j < k; j++)
		{
			printf("%u ",decode_matrix[(i * k) + j]);
		}
		printf("\n");
	}

	gf_invert_matrix(decode_matrix, invert_matrix, k);
	ec_init_tables(k, n-k, &invert_matrix[ k*k ], tables);


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
void upload(char* filename, int* fd, int* standby)
{
	printf("all the fds are:");
	for(int i=0;i<n;i++)
		printf("%d ",fd[i]);
	printf("\n");
	int len_s,len_r,len_d,max_fd=0;
	int num_of_stripes,stripe_left;
	unsigned int payload;
	unsigned int file_payload;
	unsigned char* file_data[n];
	fd_set fds;
	FILE* fp;
	size_t file_bytes;
	char * buff;
	struct message_s request;
	struct message_s reply;
	struct message_s data;
	if(access(filename,0)<0)
	{
		printf("NO EXISTING FILE\n");
		close_all_connection(fd, standby);
		exit(0);
	}
	payload=strlen(filename)+1;
	set_protocol(&request,0xC1,HEADER_LENGTH+payload);
	buff=(char*)malloc(sizeof(char)*BUFF_SIZE);
	memset(buff,'\0',sizeof(char)*BUFF_SIZE);
	strcpy(&buff[HEADER_LENGTH],filename);
	memcpy(buff,&request,HEADER_LENGTH);
	file_payload=0;
	fp=fopen(filename,"r");
	if(fp==NULL)
	{
		printf("Fail on openning file\n");
		close_all_connection(fd, standby);
		exit(0);
	}
	fseek(fp,0,SEEK_END);
	file_payload += ftell(fp);
	fseek(fp,0,SEEK_SET);
	fclose(fp);
	printf("file size is : %d\n",file_payload);
	//calculate the stripes needed;
	num_of_stripes=file_payload / (block_size * k);
	if((file_payload % (block_size * k)) > 0)
		num_of_stripes++;
	printf("total %d stripe\n",num_of_stripes);

	//caluculate the payloads for each server
	int* payloads = (int*)malloc(sizeof(int) * n);
	for(int i=0;i<k;i++)
	{
		payloads[i]=(num_of_stripes - 1) * block_size;
		if((file_payload % (block_size * k) > (block_size * i))){
			if(file_payload % (block_size * k) > (block_size * (i+1)))
				payloads[i]+=block_size;
			else
				payloads[i] += ((file_payload % (block_size *k )) - (block_size * i));
		}
		else if((file_payload / (block_size * k)) == num_of_stripes)
			payloads[i] += block_size;
		printf("payload for server %d is %d\n",i,payloads[i]);
	}
	for(int i=k;i<n;i++){
		payloads[i]=num_of_stripes * block_size;
		printf("payload for server %d is %d\n",i,payloads[i]);
	}
	//send put request to all server
	for(int i=0;i<n;i++)
	{
		if((len_s=send(fd[i],buff,HEADER_LENGTH+payload,0))==-1)
		{
			printf("Fail on PUT_REQUEST_PROTOCOL\n");
			close_all_connection(fd, standby);
			free(buff);
			exit(0);
		}
		if((len_r=recv(fd[i],&reply,HEADER_LENGTH,0))==-1)
		{
			printf("Fail on PUT_REPLY_PROTOCOL\n");
			close_all_connection(fd, standby);
			exit(0);
		}
		set_protocol(&data,0xFF,HEADER_LENGTH+payloads[i]);
		if((len_s=send(fd[i],(void*)&data,HEADER_LENGTH,0))==-1)
		{
			printf("Fail on FILE_DATA_PROTOCOL\n");
			close_all_connection(fd, standby);
			exit(0);
		}
		if(fd[i]>max_fd)
			max_fd=fd[i];		//find the max fd
	}
	printf("\n max fd is %d\n",max_fd);
	max_fd++;
	fp=fopen(filename,"r");
	if(fp==NULL)
	{
		printf("Fail on openning file\n");
		close_all_connection(fd, standby);
		exit(0);
	}
	int byte_left=file_payload;
	Stripe new_stripe;
	Stripe *stripe=&new_stripe;
	//initial the encode matrix and tables;
	uint8_t *encode_matrix=malloc(sizeof(uint8_t) *  (n*k));
	uint8_t *tables=malloc(sizeof(uint8_t) * (32 * (n-k) * k));
	printf("encode matrix:\n");
	gf_gen_rs_matrix(encode_matrix,n,k);
	ec_init_tables(k, n-k, &encode_matrix[ k*k ], tables);
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<k;j++)
		{
			printf("%u ",encode_matrix[ (i*k) + j]);
		}
		printf("\n");
	}
	//initial n chunks
	for(int i=0;i<n;i++)
	{
		file_data[i]=malloc(sizeof(unsigned char)* block_size);
	}
	stripe->data_block=file_data;
	stripe->parity_block=&file_data[k];
	stripe_left=num_of_stripes;
	//start chunking
	while(stripe_left)
	{
		//clean the stripe
		for(int i=0;i<n;i++)
			memset(file_data[i],'\0',sizeof(unsigned char) * block_size);
		// read data into a stripe
		for(int i=0;i<k;i++)
		{
			if((file_bytes=fread(file_data[i],1,block_size,fp)) < block_size)
			{
				break;
			}
		}
		ec_encode_data(block_size, k , n-k , tables , stripe->data_block , stripe->parity_block);	//encode data
		printf("stripe encoded ready to sent\n");
		for(int i=0;i<n;i++)
			printf("stripe %d chunk %d read: %s with %lu bytes\n",num_of_stripes-stripe_left+1,i+1,file_data[i],strlen(file_data[i]));	
		//use select() to send data
		int sent[n],flag=1;
		for(int i=0;i<n;i++)
		{
			sent[i]=0;
			printf("server %d sent[i] is %d payload is %d\n",i,sent[i],payloads[i]);
		}
		while(flag)
		{
			FD_ZERO(&fds);
			//set all server fd on
			for(int j=0;j<n;j++)
				FD_SET(fd[j],&fds);
			select(max_fd,NULL,&fds,NULL,NULL);
			//check each fd isset
			for(int j=0;j<n;j++)
			{
				int len=strlen(file_data[j]);
				if(FD_ISSET(fd[j],&fds) && (payloads[j]>0) && (sent[j]==0))
				{
					printf("server %d is set\nStart to send data\n",j);
					if((len_d=sendn(fd[j],file_data[j],len))==-1)
					{
						printf("Fail on sending file\n");
						close_all_connection(fd, standby);
						exit(0);
					}
					else
					{
						sent[j]=1;
						payloads[j]-=len_d;
						printf("success sent %d bytes to server : %d\n%d bytes left for this server\n",len_d,j,payloads[j]);
					}
				}
				else if(payloads[j]==0)	//if nothing to sent just set sent for ending the while loop
					sent[j]=1;
			}
			//check the stripe was sent
			int sum=0;
			for(int j=0;j<n;j++)
				sum+=sent[j];
			//printf("\nsum is %d\n",sum);
			if(sum == n)
				flag=0;
		}
		stripe_left--;
	}
	fclose(fp);
	free(encode_matrix);
	free(tables);
	for(int i=0;i<n;i++)
	{
		free(file_data[i]);
	}
	free(buff);
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
		if(strcmp(argv[2],"put")==0)
		{
			flag=PUT_REQUEST;
		}
		else if(strcmp(argv[2],"get")==0)
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

	// read config from config file
	FILE *client_config;
	client_config=fopen(argv[1],"r");
	if(client_config == NULL)
	{
		printf("client config file NOT EXIST!\n");
		exit(0);
	}
	fscanf(client_config,"%d %d %d",&n,&k,&block_size);
    fgetc(client_config);
	printf("client config info:\n");
    printf("n= %d\nk= %d\nblock_size= %d\n",n,k,block_size);
	if(block_size>CHUNK_SIZE)
	{
		printf("block_size is out of size!\n");
		exit(0);
	}

	char** ip_addr = (char**)malloc(sizeof(char) * n * 25);
	int *ports = (int*)malloc(sizeof(int) * n);
	char line[25];
	for(int i = 0; i < n; i++){
		ip_addr[i] = malloc(25);
		fscanf(client_config, "%s", line);
		strcpy(ip_addr[i], strtok(line, ":\n"));
		sscanf(strtok(NULL, ":\n"), "%d", &ports[i]);
        printf("server ip %d : %s : %d\n",i+1,ip_addr[i],ports[i]);		//print info
	}
	fclose(client_config);

	//build connection to servers
	int* fd = (int*)malloc(sizeof(int) * n);
	int* standby = (int*)malloc(sizeof(int) * n);			//standby[n] is recorded wheter the server is connected successful,0 is fail, 1 is success
	int all_on=1;					//set all server is on

	// create connection and check standby[]
	for(int i = 0; i < n ; i ++)
	{
		int addrlen=sizeof(struct sockaddr_in);
		struct sockaddr_in addr;
		in_addr_t ip=inet_addr(ip_addr[i]);
		unsigned short port=ports[i];
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
			printf("connection failed with server %d\n",i);
			perror("connect()");
			close(fd[i]);
			standby[i]=0;
			all_on=0;			//any one of the server was not connected, all on =0
		}
		else
		{
			printf("connected with server %d\n",i);
			standby[i]=1;
		}
	}
	int fake_fd=0;

	// decide what to do according to user input
	// main logic
	switch(flag)
	{
		case LIST_REQUEST:
			list(fd,standby);
			break;
		case GET_REQUEST:
			download(argv[3], fd, fake_fd, standby);
			break;
		case PUT_REQUEST:
			if(all_on) upload(argv[3],fd, standby);
			else{
				printf("Servers are not all connected!!\n");
				close_all_connection(fd, standby);
			}
			break;
		default:
			printf("unknown command!\n");
			break;
	}
	
	return 0;
}
