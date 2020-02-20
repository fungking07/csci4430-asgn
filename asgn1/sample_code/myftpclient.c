#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "myftp.h"

#define N_F_PATH "N_F_PATH"

/*
 *  Function: list_file
 *  -------------------
 *  
 *  fd: a void pointer pointing to the address of client socket.
 *  
 *  returns: nothing
 * 
*/

void list_file(int fd)
{
    printf("--------------------------------------------------------\n");
    printf("Server, what is the list of avilable files in this moment?\n");
    printf("--------------------------------------------------------\n\n");
    int s_len, r_len;
    char* buffer;
    struct message_s LIST_REQUEST;
    struct message_s LIST_REPLY;

    set_protocol(&LIST_REQUEST, 0xA1, HEADER_LENGTH);

    /* send LIST_REQUEST protocol to server */
    printf("Sending LIST_REQUEST protocol to server...............\n");
    if ((s_len = send(fd, (void*)&LIST_REQUEST, sizeof(LIST_REQUEST), 0)) == -1) {
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to send LIST_REQUEST protocol to server\n");
        printf("\n**********************************\n");
        close(fd);
        exit(0);
    }
    else {
        printf("Success to send LIST_REQUEST protocol to server!!!\n\n");
        // printf("DEBUG: Length of LIST_REQUEST: %d\n", s_len);
    }

    /* recv buff from the server */

    buffer = (char*)malloc(sizeof(char)*BLOCK);
    memset(buffer, '\0', sizeof(char)*BLOCK);

    printf("Receiving message from server...............\n");
    if((r_len = recv(fd, buffer, BLOCK, 0)) == -1) {
        printf("**********ERROR MESSAGES**********\n\n");
        printf("Fail to recv LIST_REPLY protocol from server\n");
        printf("\n**********************************\n");
        close(fd);
        exit(0);
    }
    else {
        printf("Success to receive message from server!!!\n\n");
        // printf("DEBUG: Length of LIST_REPLY buffer : %d\n", r_len);
    }

    /* extract protocol from buff and check the protocol type */

    memcpy(&LIST_REPLY, buffer, HEADER_LENGTH);
    // printf("DEBUG: LIST_REPLY.protocol: %s\n",LIST_REPLY.protocol);
    // printf("DEBUG: LIST_REPLY.length: %d\n", LIST_REPLY.length);
	// printf("DEBUG: LIST_REPLY.type: %x\n", LIST_REPLY.type);
    // printf("DEBUG: LIST_REPLY.type: %d\n", r_len);

    printf("Checking protocol...............\n");
    if(get_protocol_type(LIST_REPLY, r_len) == LIST_REPLY_PROTOCOL) {
        printf("Success to identify LIST_REPLY protocol from message!!!\n\n");
        printf("---------------------------------\n");
        printf("List of avilable files in server:\n");
        printf("---------------------------------\n");        
        printf("%s", &buffer[HEADER_LENGTH]);   // extract the result from buff and print it.
        free(buffer);
    }
    else {
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to identify LIST_REPLY protocol from message!!!!\n");
        printf("\n**********************************\n");
    }

}

/*
 *  Function: file_download (NOT COMPLETE)
 *  -------------------
 *  
 *  fd: a file descriptor of client socket.
 *  path: a char pointer pointing to the argv[4] 
 * 
 *  returns: nothing
 * 
*/

void file_download(int fd, char* path)
{
    // printf("DEBUG: Length of file name: %ld\n", strlen(path));

    printf("--------------------------------------------------------\n");
    printf("Server, I want to download file!!!\n");
    printf("--------------------------------------------------------\n\n");
    
    int s_len, r_len;
    unsigned int filesize;
    unsigned int payload;
    char* buffer;
    char *file_buffer;
    FILE *fp;
    struct message_s GET_REQUEST;
    struct message_s GET_REPLY;
    struct message_s FILE_DATA;

    /* set the protocol */

    payload = strlen(path) + 1;
    set_protocol(&GET_REQUEST, 0xB1, HEADER_LENGTH + payload);

    /* combine the protocol and the file path into buffer and send the buffer*/

    buffer = (char*)malloc(sizeof(char)*BLOCK);
    memset(buffer, '\0', sizeof(char)*BLOCK);
    strcpy(&buffer[HEADER_LENGTH], path);
    memcpy(buffer, &GET_REQUEST, HEADER_LENGTH);

    printf("Sending GET_REQUEST protocol to server...............\n");
    if ((s_len = send(fd, buffer, HEADER_LENGTH + payload, 0)) == -1) {
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to send GET_REQUEST protocol to server\n");
        printf("\n**********************************\n");
        exit(0);
    }
    else {
        free(buffer);
        printf("Success to send GET_REQUEST protocol to server!!!\n\n");
        // printf("DEBUG: Length of message : %d\n", s_len);
    }

    /* recv GET_REPLY protocol and check its type */

    printf("Receiving message from server...............\n");
    if((r_len = recv(fd, &GET_REPLY, HEADER_LENGTH, 0)) == -1) {
        printf("**********ERROR MESSAGES**********\n\n");
        printf("Fail to recv GET_REPLY protocol from server\n");
        printf("\n**********************************\n");
        exit(0);
    }
    else {
        printf("Success to receive message from server!!!\n\n");
        // printf("DEBUG: Length of GET_REPLY : %d\n", s_len);
    }

    printf("Checking protocol...............\n");
    if(get_protocol_type(GET_REPLY, r_len) == GET_REPLY_PROTOCOL_EXISTS) {
        printf("Success to identify GET_REPLY protocol from message!!! Type: FILE_EXIST!!!\n");
        printf("File exists in server!!! Let's get it!!!\n\n");

        /* recv FILE_DATA protocol */
        printf("Receiving message from server...............\n");
        if((r_len = recv(fd, &FILE_DATA, sizeof(FILE_DATA), 0)) == -1) {
            printf("**********ERROR MESSAGES**********\n\n");
            printf("Fail to recv FILE_DATA protocol from server\n");
            printf("\n**********************************\n");
            exit(0);
        }
        else {
            // printf("DEBUG: Length of FILE_DATA : %d\n", r_len);
            // printf("DEBUF: FILE_DATA.protocol: %s\n", FILE_DATA.protocol);
            // printf("DEBUF: FILE_DATA.type: %x\n", FILE_DATA.type);
            printf("Success to receive FILE_DATA from server!!!\n");
            printf("Size of the file: %d bytes\n\n", ntohl(FILE_DATA.length));
        }

    }
    else if(get_protocol_type(GET_REPLY, r_len) == GET_REPLY_PROTOCOL_NOT_EXISTS) {
        printf("Success to identify GET_REPLY protocol from message!!! Type: FILE_NOT_EXIST!!!\n");
        printf("File not exists in server!!! Unhappy!!!\n\n");
        exit(0);
    }
    else {
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to identify any protocol from message!!!!\n\n");
        printf("\n**********************************\n");
        exit(0);
    }

    /* recv the file chunks by chunks */

    filesize = ntohl(FILE_DATA.length) - HEADER_LENGTH;
    file_buffer = (char*)malloc(sizeof(char) * FILE_SIZE);
    fp = fopen(path, "wb"); 

    printf("Receiving file from server...............\n");
    if((r_len = recvn(fd, file_buffer, filesize)) == -1) {  // make sure all bytes are received.
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to receive file from server!!!\n\n");
        printf("\n**********************************\n");
        exit(0);
    }
    else {
        // printf("DEBUG: Length of file chunks : %d\n", r_len);
    }

    printf("Processing...............\n");
    printf("Please wait..........\n\n");

    fwrite(file_buffer, 1, r_len, fp);  // write to file
    fclose(fp);

    printf("Success to get the file from server!!!\n\n");

}

/*
*  Function: file_upload (NOT COMPLETE)
*  -------------------
*
*/

void file_upload(int fd, char* path)
{
    printf("--------------------------------------------------------\n");
    printf("Server, I want to upload file!!!\n");
    printf("--------------------------------------------------------\n\n");

	int s_len, r_len;
	unsigned int payload;
	int file_payload;
	char* buffer;
	char* file_content;
	FILE* file;
	size_t file_bytes;
	struct message_s PUT_REQUEST;
	struct message_s PUT_REPLY;
	struct message_s FILE_DATA;

	// checking file exist or not
	if (access(path, 0) < 0) {
		printf("File does not exist in client server!!!\n");
		exit(0);
	}
	else {
		/* set the protocol */

		payload = strlen(path) + 1;
		set_protocol(&PUT_REQUEST, 0xC1, HEADER_LENGTH + payload);

		/* combine the protocol and the file path into buffer and send the buffer*/

		buffer = (char*)malloc(sizeof(char)*BLOCK);
		memset(buffer, '\0', sizeof(char)*BLOCK);
		strcpy(&buffer[HEADER_LENGTH], path);
		memcpy(buffer, &PUT_REQUEST, HEADER_LENGTH);

        printf("Sending PUT_REQUEST message to server...............\n");
		if ((s_len = send(fd, buffer, HEADER_LENGTH + payload, 0)) == -1) {
			printf("**********ERROR MESSAGES**********\n\n");
            printf("Fail to recv PUT_REQUEST protocol from server\n");
            printf("\n**********************************\n");
			exit(0);
		}
		else {
            printf("Success to send PUT_REQUEST message to server!!!\n\n");
			free(buffer);
			// printf("DEBUG: Length of message : %d\n", s_len);
		}

		/* recv PUT_REPLY protocol */

        printf("Receiving message from server...............\n");
		if ((r_len = recv(fd, &PUT_REPLY, HEADER_LENGTH, 0)) == -1) {
			printf("**********ERROR MESSAGES**********\n\n");
            printf("Fail to recv PUT_REPLY message from server\n");
            printf("\n**********************************\n");
			exit(0);
		}
		else {
            printf("Success to receive message from server!!!\n\n");
			// printf("DEBUG: Length of message : %d\n", s_len);
		}

        file_payload = 0;
        file = fopen(path, "r");
        fseek(file, 0, SEEK_END);
        file_payload += ftell(file);
        fseek(file, 0, SEEK_SET);
        fclose(file);

        /* send FILE_DATA protocol to server */

        printf("File size = %d\n\n", file_payload);
        set_protocol(&FILE_DATA, 0xFF, HEADER_LENGTH + file_payload);

        printf("Sending FILE_DATA protocol to server...............\n");
        if ((s_len = send(fd, &FILE_DATA, HEADER_LENGTH, 0)) == -1) {
            printf("\n**********ERROR MESSAGES**********\n\n");
            printf("Fail to send data FILE_DATA protocol to server\n");
            printf("\n**********************************\n");
            exit(0);
        }
        else {
            printf("Success to send data FILE_DATA protocol to server\n\n");
            // printf("DEBUG: Length of FILE_DATA : %d\n", s_len);
        }

        /*send file to server*/

        file_bytes = 0;
        file_content = (char*)malloc(sizeof(char) * FILE_SIZE);
        file = fopen(path, "r");

        int no_of_byte_left = file_payload;
        printf("Sending file to client chunks by chunks...............\n\n");
        printf("No of bytes left: %d\n", no_of_byte_left);
        while ((file_bytes = fread(file_content, 1, CHUNK_SIZE, file)) > 0)
        {
            if ((s_len = sendn(fd, file_content, file_bytes)) == -1) {
                printf("\n**********ERROR MESSAGES**********\n\n");
                printf("Fail to send file to server!!!\n");
                printf("\n**********************************\n");
            }
            else {
                no_of_byte_left -= file_bytes;
                printf("No of bytes left: %d\n", no_of_byte_left);
            }
        }
	}

}



/*
 *  Function: main_loop
 *  -------------------
 *  
 *  ip: ip address of the server, with network byte ordering
 *  port: port number of the server, with default uint16_t type
 *  f_path: requested file path, if the path is not needed by the operation,
 *          by default it would be N_F_PATH, which declare at the begining.
 *  operation:  represent client request, go to myftp.h for more detail. 
 * 
 *  returns: nothing
 * 
*/

void main_loop(in_addr_t ip, unsigned short port, char* f_path, const int operation)
{
    int c_socket_fd;
    int len;
    unsigned int addrlen;
    struct sockaddr_in server_address;

    // printf("File path: %s\n", f_path);
    // printf("DEBUG: Operation: %d\n", operation);

    /* create socket */

    printf("Creating socket for client...............\n");
    if ((c_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to create socket for client!!!\n");
        printf("\n**********************************\n");
    }
    else {
        printf("Success to create socket for client!!!\n\n");
    }

    /* connect() */

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = ip;
    server_address.sin_port = htons(port);
    addrlen = sizeof(struct sockaddr_in);

    printf("Connecting socket to server...............\n");    
    if (connect(c_socket_fd, (struct sockaddr *)&server_address, addrlen) == -1) {
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to connect socket to server!!!\n");
        printf("Connection error: %s (Errno:%d)\n", strerror(errno), errno);
        printf("\n**********************************\n");
        close(c_socket_fd);
        exit(0);
    } 
    else {
        printf("Success to connect socket to server!!!\n\n");
        printf("Establish connection to server!!!\n\n");
    }
    
    /* Handle and send client request to server */

    if(operation == LIST_REQUEST_PROTOCOL) {
        list_file(c_socket_fd);
    }
    else if(operation == GET_REQUEST_PROTOCOL) {
        file_download(c_socket_fd, f_path);
    }
	else if (operation == PUT_REQUEST_PROTOCOL) {
		file_upload(c_socket_fd, f_path);
	}

	close(c_socket_fd);
}

/*
 *  Function: main
 *  -------------------
 *  
 *  argc: number of argument.
 *  argv: pointer pointing to the address of char arrays containing various argument.
 * 
 *  returns: nothing
 * 
*/

int main(int argc, char **argv)
{
    printf("\n");
    if(argc < 4) {
        printf("\n**********ERROR MESSAGES**********\n");
        printf("Invalid command to start the client\nUsage: /myftpclient [server ip addr] [server port] [list|get|put] [file]\n");
        printf("Number of argument is less than 4!\n");
    }
    else if(argc == 4) {

        if(strcmp(argv[3], "list") == 0) {
            main_loop(inet_addr(argv[1]), atoi(argv[2]), N_F_PATH, LIST_REQUEST_PROTOCOL);
        }
        else if (strcmp(argv[3], "get") == 0) {
            printf("\n**********ERROR MESSAGES**********\n\n");
            printf("Invalid command to perform get\nPlease provide file path information!\n");
            printf("\n**********************************\n");
        }
		else if (strcmp(argv[3], "put") == 0) {
            printf("\n**********ERROR MESSAGES**********\n\n");
			printf("Invalid command to perform get\nPlease provide file path information!\n");
            printf("\n**********************************\n");
		}
    }
    else if(argc == 5) {

        if(strcmp(argv[3], "get") == 0) {
            main_loop(inet_addr(argv[1]), atoi(argv[2]), argv[4], GET_REQUEST_PROTOCOL);
        }
		else if (strcmp(argv[3], "put") == 0) {
			main_loop(inet_addr(argv[1]), atoi(argv[2]), argv[4], PUT_REQUEST_PROTOCOL);
		}
        else if (strcmp(argv[3], "list") == 0) {
			printf("\n**********ERROR MESSAGES**********\n\n");
			printf("Too much argument for list file operation!\n");
            printf("\n**********************************\n");
		}
    }
    else {
        printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Invalid command to start the client\nUsage: /myftpclient [server ip addr] [server port] [list|get|put] [file]\n");
        printf("Number of argument MAYBE more than 5!");
        printf("\n**********************************\n");
    }
}