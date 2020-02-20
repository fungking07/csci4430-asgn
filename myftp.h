#define HEADER_LENGTH 10 //char[5]+char+int=5+1+4
#define BUFF_SIZE 1024  // default size malloc() to a pointer variable.

#define LIST_REQUEST 0
#define GET_REQUEST 1
#define PUT_REQUEST 2
#define FILE_DATA 3
#define LIST_REPLY 4
#define GET_REPLY_EXISTS 5
#define GET_REPLY_NOT_EXISTS 6
#define PUT_REPLY 7

#define CHUNK_SIZE 1024000
#define MAX_SIZE 1073741825

struct message_s{
    unsigned char protocol[5];
    unsigned char type;
    unsigned int length;
}__attribute__((packed));

void set_protocol(struct message_s *protocol, unsigned char type, unsigned int length);

int type_to_int(struct message_s protocol, unsigned int len);

int sendn(int sd,void *buf,int len);

int recvn(int sd,void *buf,int len);