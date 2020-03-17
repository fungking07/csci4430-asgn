/*
    myftp.h and myftp.c handling common constants and function shared by both myftpclient.c and myftpserver.c
*/

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include "myftp.h"

void set_protocol(struct message_s *protocol, unsigned char type, unsigned int length)
{
    strncpy(protocol->protocol, "myftp", 5);
    protocol->type = type;
    protocol->length = ntohl(length);
}

int get_protocol_type(struct message_s ptc, unsigned int length)
{
    // printf("Length: %d\n", length);
    // printf("DEBUG: CHECKPOINT 1\n");
    if(strncmp(ptc.protocol, "myftp", 5) == 0)
    {
        /* LIST REQUEST */

        // printf("DEBUG: CHECKPOINT 2\n");
        
        if(ptc.type == 0xA1) {
            // printf("DEBUG: CHECKPOINT 3\n");
            if(ptc.length == ntohl(length)) {
                // printf("DEBUG: CHECKPOINT 4\n");
                return LIST_REQUEST_PROTOCOL;
            }
            else {
                return -1;
            }
        }

        /* LIST REPLY */

        else if(ptc.type == 0xA2) {
            // printf("DEBUG: CHECKPOINT 3\n");
            if(ptc.length == ntohl(length)) {
                // printf("DEBUG: CHECKPOINT 4\n");
                // printf("DEBUG: length: %d\n", length);
                // printf("DEBUG: ntohl(ptc.length): %d", ntohl(ntohl(ptc.length)));
                return LIST_REPLY_PROTOCOL;
            }
            else {
                return -1;
            }
        }

        /* GET REQUEST */

        else if(ptc.type == 0xB1) {
            // printf("DEBUG: CHECKPOINT 3\n");
            // printf("DEBUG: ptc.length: %d\n", ptc.length);
            // printf("DEBUG: length: %d\n", length);
            // printf("DEBUG: ntohl(ptc.length): %d", ntohl(ptc.length));
            // printf("DEBUG: ntohl(length): %d", ntohl(length));
            if(ptc.length == ntohl(length)) {
                // printf("DEBUG: CHECKPOINT 4\n");
                return GET_REQUEST_PROTOCOL;
            }
            else {
                return -1;
            }
        }

        /* GET REPLY: file exists */

        else if(ptc.type == 0xB2) {
            // printf("DEBUG: CHECKPOINT 3\n");
            // printf("DEBUG: ptc.length: %d\n", ptc.length);
            // printf("DEBUG: length: %d\n", length);
            // printf("DEBUG: ntohl(ptc.length): %d", ntohl(ptc.length));
            // printf("DEBUG: ntohl(length): %d", ntohl(length));
            if(ptc.length == ntohl(length)) {
                // printf("DEBUG: CHECKPOINT 4\n");
                return GET_REPLY_PROTOCOL_EXISTS;
            }
            else {
                return -1;
            }
        }

        /* GET REPLY: file not exists */

        else if(ptc.type == 0xB3) {
            // printf("DEBUG: CHECKPOINT 3\n");
            // printf("DEBUG: ptc.length: %d\n", ptc.length);
            // printf("DEBUG: length: %d\n", length);
            // printf("DEBUG: ntohl(ptc.length): %d", ntohl(ptc.length));
            // printf("DEBUG: ntohl(length): %d", ntohl(length));
            if(ptc.length == ntohl(length)) {
                // printf("DEBUG: CHECKPOINT 4\n");
                return GET_REPLY_PROTOCOL_NOT_EXISTS;
            }
            else {
                return -1;
            }
        }

        /* PUT REQUEST */

        else if(ptc.type == 0xC1) {
            if(ptc.length == ntohl(length)) {
                return PUT_REQUEST_PROTOCOL;
            }
            else {
                return -1;
            }
        }
		/* PUT REPLY */
		else if (ptc.type == 0xC2) {
			if (ptc.length == ntohl(length)) {
				return PUT_REPLY_PROTOCOL;
			}
			else {
				return -1;
			}
		}
        /* FILE DATA */

        else if(ptc.type == 0xFF) {
			// printf("DEBUG: CHECKPOINT 1\n");
            // printf("DEBUG: ptc.length: %d\n", ntohl(ptc.length));
            // printf("DEBUG: ntohl-ptc.length: %d\n", ptc.length);

            // printf("DEBUG: ntohl-length: %d\n", ntohl(length));
            // printf("DEBUG: length: %d\n", length);

			// printf("DEBUG: ptc.protocol: %s\n", ptc.protocol);
			// printf("DEBUG: ptc.type: %x\n", ptc.type);
            // printf("DEBUG: %ld\n", sizeof(ptc));
            if(ntohl(sizeof(ptc)) == ntohl(length)) {
                // printf("DEBUG: CHECKPOINT 2\n");
                return FILE_DATA_PROTOCOL;
            }
            else {
                // printf("DEBUG: CHECKPOINT 3\n");
                return -1;
            }
        }
		
    }
    else {
        // printf("DEBUG: CHECKPOINT 4\n");
        return -1;
    }
}

int sendn(int fd, void *buff, int buff_len)
{
    int n_left = buff_len;
    int n;
    while(n_left > 0)
    {
        if((n = send(fd, buff + (buff_len - n_left), n_left, 0)) < 0) 
        {
            // printf("DEBUG: file_bytes: %d\n", n);
            if(errno == EINTR) {
                // printf("DEBUG: EINTR\n");
                n = 0;
            }
            else {
                // printf("DEBUG: -1\n");
                printf("**********ERROR MESSAGES**********\n\n");
                printf("error: %s (Errno:%d)\n", strerror(errno), errno);
                printf("\n**********************************\n");		

                return -1;
            }
        } 
        else if(n == 0) {
            return 0;
        }
        n_left -= n;
    }
    return buff_len;
}

int recvn(int fd, void *buff, int buff_len)
{
    int n_left = buff_len;
    int n;
    while(n_left > 0) 
    {
        if((n = recv(fd, buff + (buff_len - n_left), n_left, 0)) == -1) 
        {
            if(errno == EINTR) {
                n = 0;
            }
            else {
                printf("**********ERROR MESSAGES**********\n\n");
                printf("error: %s (Errno:%d)\n", strerror(errno), errno);
                printf("\n**********************************\n");		
                return -1;
            }
        }
        else if (n == 0) {
            return 0;
        }
        // printf("No of bytes left: %d\n", n_left);
        n_left -= n;
    }
    return buff_len;
}