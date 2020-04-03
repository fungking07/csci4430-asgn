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

//set the struct message_s
void set_protocol(struct message_s *protocol, unsigned char type, unsigned int len)
{
	strncpy(protocol->protocol,"myftp",5);
	protocol->type=type;
	protocol->length=ntohl(len);
}

// check protocol
// return the type of action
int type_to_int(struct message_s protocol,unsigned int len)
{
	if((strncmp(protocol.protocol,"myftp",5)==0)&&(protocol.length==ntohl(len)))
	{
		if(protocol.type==0xA1)
			return LIST_REQUEST;			//0
		else if(protocol.type==0xA2)
			return LIST_REPLY;				//4
		else if(protocol.type==0xB1)
			return GET_REQUEST;				//1
		else if(protocol.type==0xB2)
			return GET_REPLY_EXISTS;		//5
		else if(protocol.type==0xB3)
			return GET_REPLY_NOT_EXISTS;	//6
		else if(protocol.type==0xC2)
			return PUT_REPLY;				//7
		else if(protocol.type==0xFF)
			return FILE_DATA;				//3
		else if(protocol.type==0xC1)
			return PUT_REQUEST;				//2
		else
			return -1;
	}
	else
	{
		return -1;
	}
	
}

int sendn(int sd,void *buf,int len)
{
	int n_left=len;
	int n;
	while(n_left>0)
	{
		if((n=send(sd,buf+(len-n_left),n_left,0))<0)
		{
			if(errno == EINTR)
				n=0; //EINTR: interrupt
			else
			{
				printf("error:%s (Errno:%d)\n",strerror(errno),errno);
				return -1;
			}
		}
		else if(n == 0)
		{
			return 0;
		}
		n_left -= n;
	}
	return len;
}

int recvn(int sd,void *buf,int len)
{
	int n_left=len;
	int n;
	while(n_left>0)
	{
		if((n=recv(sd,buf+(len-n_left),n_left,0))<0)
		{
			if(errno == EINTR)
				n=0; //EINTR: interrupt
			else
			{
				printf("error:%s (Errno:%d)\n",strerror(errno),errno);
				return -1;
			}
		}
		else if(n == 0)
		{
			return 0;
		}
		n_left -= n;
	}
	return len;
}
