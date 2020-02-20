/*
    myftp.h and myftp.c handling common constants and function shared by both myftpclient.c and myftpserver.c
*/

#ifndef MYFTP_H_    // include guard
#define MYFTP_H_

#define HEADER_LENGTH 10    // 5 + 1 + 4
#define BLOCK 1024  // default size malloc() to a pointer variable.

/* Types of protocol */
#define LIST_REQUEST_PROTOCOL 1
#define GET_REQUEST_PROTOCOL 2
#define PUT_REQUEST_PROTOCOL 3
#define FILE_DATA_PROTOCOL 4
#define LIST_REPLY_PROTOCOL 5
#define GET_REPLY_PROTOCOL_EXISTS 6
#define GET_REPLY_PROTOCOL_NOT_EXISTS 7
#define PUT_REPLY_PROTOCOL 8

/* file */
#define CHUNK_SIZE 1024000
#define FILE_SIZE 1073741825

struct message_s {
    unsigned char protocol[5];  // 5 bytes
    unsigned char type;         // 1 bytes
    unsigned int length;        // header + payload, 4 bytes
} __attribute__ ((packed));

/*
 *  Function: set_protocol 
 *  -------------------
 *  
 *  protocol: a void pointer pointing to the address of protocol being issuses.
 *  type:   protocol type
 *  length: protocol length
 * 
 *  returns: nothing  
 * 
*/
void set_protocol(struct message_s *protocol, unsigned char type, unsigned int length);

/*
 *  Function: get_protocol_type (NOT COMPLETE)
 *  -------------------
 *  
 *  ptc: the protocol being checked, a message_s struct variable 
 *  length: return value of recv().
 * 
 *  returns: type of the protocol, -1 means fail.
 * 
 *  TODO: use protocol reference as parms instead of struct variable to enhance performance
 * 
*/
int get_protocol_type(struct message_s ptc, unsigned int length);

/*
 *  Function: sendn
 *  -------------------
 *  
 *  fd: a file descriptor of client socket
 *  buff: a void pointer for storing the data
 *  buff_len: length are required to receive
 * 
*/
int sendn(int fd, void *buff, int buff_len);

/*
 *  Function: recvn 
 *  -------------------
 *  
 *  fd: a file descriptor of client socket
 *  buff: a void pointer for storing the data
 *  buff_len: length are required to receive
 * 
*/
int recvn(int fd, void *buff, int buff_len);

#endif