#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include "myftp.h"

#define FILE_DIR "data/"

/*
*  Function: list_file
*  -------------------
*
*  recv_socket_fd: a void pointer pointing to the address of client socket.
*
*  returns: a pointer called buffer containing protocol LIST_REPLY at the beginning and
*           a list of file in data folder at the end.
*
*/

void list_file(void *recv_socket_fd)
{

	printf("--------------------------------------------------------------\n");
    printf("Boss said found the list of avilable files as soon as possible\n");
    printf("--------------------------------------------------------------\n\n");

	int len;
	unsigned int payload;
	int *c_socket_fd;
	char* buffer;
	DIR* dir;

	#ifdef sun
        struct dirent64 entry;
		struct dirent64 *file_name = &entry;
    #elif __sun
        struct dirent64 entry;
		struct dirent64 *file_name = &entry;
	#else
		struct dirent entry;
		struct dirent *file_name = &entry;
	#endif

	struct message_s LIST_REPLY;

	c_socket_fd = (int*)recv_socket_fd;

	buffer = (char*)malloc(sizeof(char)*BLOCK);
	memset(buffer, '\0', sizeof(char)*BLOCK);

	/* get the list of file names */
	payload = 0;
	dir = opendir(FILE_DIR);

    printf("Processing...............\n");
	while (1) {
		int result = readdir_r(dir, file_name, &file_name);

		if (result == 0) {
			if (file_name == NULL) {
				break;
			}
			else {
				// printf("DEBIG: filename: %s\n", entry.d_name);
				/* load the payload into the position right after the control protocol of the buffer */
				strcpy(&buffer[payload + HEADER_LENGTH], entry.d_name);
				// printf("DEBUG: len: %ld\n", strlen(entry.d_name) + 1);
				payload += strlen(entry.d_name) + 1;
				strcpy(&buffer[payload + HEADER_LENGTH - 1], "\n");
			}
		}
		else {
			printf("\n**********ERROR MESSAGES**********\n\n");
			printf("Fail to read data directory:%s\n", strerror(errno));
			printf("\n**********************************\n");
			exit(0);
		}
	}
	closedir(dir);
	printf("List of avilable file found!!!\n\n");

	set_protocol(&LIST_REPLY, 0xA2, HEADER_LENGTH + payload);   // initialize LIST_REPLY protocols
	memcpy(buffer, &LIST_REPLY, HEADER_LENGTH); // load the protocol into the buffer at the beginning position

	/* send whole buffer contains both the protocol and the results requested by client. */
	// printf("payload: %d\n", payload);
    printf("Sending LIST_REPLY protocol to client...............\n");
    printf("Sending list of avilable file to client...............\n");
	if ((len = send(*c_socket_fd, buffer, HEADER_LENGTH + payload, 0)) == -1) {
		printf("\n**********ERROR MESSAGES**********\n\n");
		printf("Fail to send data LIST_REPLY protocol to client!!!\n");
		printf("Fail to send list of avilable file to client!!!\n");
		printf("\n**********************************\n");
		exit(0);
	}
	else {
		printf("Success to send data LIST_REPLY protocol to client!!!\n");
		printf("Success to send list of avilable file to client!!!\n\n");
		free(buffer);
		// printf("DEBUG: Length of message : %d\n", len);
	}

	printf("\n-----------------------------------------\n");
    printf("All things done!!! Afternoon tea times!!!\n");
    printf("-----------------------------------------\n\n");

}

/*
*  Function: download_file (NOT COMPLETE)
*  -------------------
*
*  recv_socket_fd: a void pointer pointing to the address of client socket.
*  request_file: a char pointer pointer to address storing the content of file name.
*
*  returns: nothing
*
*/

void download_file(void *recv_socket_fd, char* request_file)
{

	printf("--------------------------------------\n");
    printf("Boss said he wants to download file!!!\n");
    printf("--------------------------------------\n\n");

	int len;
    int payload;
    int *c_socket_fd;
    char* full_request_file_path;
    char* file_content;
    FILE* file;
    struct message_s GET_REPLY;
    struct message_s FILE_DATA;
	int file_bytes;

    c_socket_fd = (int*) recv_socket_fd;
    
    /* construct full path from the file name */

	

    full_request_file_path = (char*)malloc(strlen(FILE_DIR) + strlen(request_file) + 1);
    strcpy(full_request_file_path, FILE_DIR);
    strcat(full_request_file_path, request_file);

    // printf("DEBUG: Full path of the file: %s\n", full_request_file_path);

    /*  check the file is exists or not by checking the errno return by fopen() 
     *  construct GET_REPLY protocol, 0xB2 = exists, 0xB3 = not exists
    */

	if (access(full_request_file_path, 0) < 0) {
		printf("What? File does not exist in server!!!\n\n");
		set_protocol(&GET_REPLY, 0xB3, HEADER_LENGTH);
    	printf("Sending GET_REPLY protocol to client. Type: NOT_EXIST...............\n");
		if((len = send(*c_socket_fd, &GET_REPLY, HEADER_LENGTH, 0)) == -1) {
			printf("\n**********ERROR MESSAGES**********\n\n");
			printf("Fail to send GET_REPLY protocol message to client!!! Type: NOT_EXIST\n");
			printf("\n**********************************\n");
			exit(0);
		}
		else {
			printf("Success to send GET_REPLY protocol to client. Type: NOT_EXIST\n\n");
		}

		printf("\n-----------------------------------------\n");
		printf("No file!!! Yeah!!! Afternoon tea times!!!\n");
		printf("-----------------------------------------\n\n");

		return;
	}
	else{

		payload = 0;
		file = fopen(full_request_file_path, "r");
		if(file) {
			fseek(file, 0, SEEK_END);
			payload += ftell(file);
			fseek(file, 0, SEEK_SET);
			printf("File exists in server!!!\n\n");
			set_protocol(&GET_REPLY, 0xB2, HEADER_LENGTH);
			fclose(file);
		}
		else {
			if(errno == 2) {    // Errno 2: No such file or directory
				// printf("File %s doesn't exist in server\n", request_file);
			}
			else {
				// No such process or Permission denied or ....
			printf("\n**********ERROR MESSAGES**********\n\n");
			printf("Permission denied for opening the file!!!");
			printf("\n**********************************\n");				exit(1);
			}
		}

		/* send GET_REPLY protocol to client to see whether the file is exists or not*/ 
    	printf("Sending GET_REPLY protocol to client. Type: EXISTS...............\n");
		if((len = send(*c_socket_fd, &GET_REPLY, HEADER_LENGTH, 0)) == -1) {
			printf("\n**********ERROR MESSAGES**********\n\n");
			printf("Fail to send GET_REPLY protocol message to client!!! Type: EXIST\n");
			printf("\n**********************************\n");
			exit(0);
		}
		else {
			printf("Success to send GET_REPLY protocol to client. Type: EXISTS\n\n");
			// printf("DEBUG: Length of GET_REPLY : %d\n", len);
		}
		

		/* send FILE_DATA protocol to client */

		if(GET_REPLY.type == 0xB2)
		{
			// printf("File size = %d\n", payload);
			// printf("DEBUF: HEADER_LENGTH + payload: %d\n", HEADER_LENGTH + payload);
			set_protocol(&FILE_DATA, 0xFF, HEADER_LENGTH + payload);

			// printf("DEBUF: FILE_DATA.protocol: %s\n", FILE_DATA.protocol);
			// printf("DEBUF: FILE_DATA.type: %x\n", FILE_DATA.type);
			printf("File size = %d\n\n", ntohl(FILE_DATA.length));

    		printf("Sending FILE_DATA protocol to client...............\n");
			if((len = send(*c_socket_fd, &FILE_DATA, sizeof(FILE_DATA), 0)) == -1) {
				printf("\n**********ERROR MESSAGES**********\n\n");
				printf("Fail to send data FILE_DATA protocol to client\n");
				printf("\n**********************************\n");
				exit(0);
			}
			else {
				printf("Success to send data FILE_DATA protocol to client\n\n");
				// printf("DEBUG: Length of FILE_DATA : %d\n", len);
			}
		}
		else {   
			return;
		}

		/* send file to client */

		file_bytes = 0;
		file_content = (char*) malloc(sizeof(char) * FILE_SIZE);
		file = fopen(full_request_file_path, "r");

		int no_of_byte_left = ntohl(FILE_DATA.length);
	    printf("Sending file to client chunks by chunks...............\n");
		while((file_bytes = fread(file_content, 1, CHUNK_SIZE, file)) > 0)
		{
			// printf("DEBUG: file_bytes: %d\n", file_bytes);
			if((len = sendn(*c_socket_fd, file_content, file_bytes)) == -1) {
				printf("\n**********ERROR MESSAGES**********\n\n");
				printf("Fail to send file to client!!!\n");
				printf("\n**********************************\n");
			}
			else {
				no_of_byte_left -= file_bytes;
				printf("No of bytes left: %d\n", no_of_byte_left);
			}
			// printf("Size of file chunks: %d\n", len);
		}

		free(file_content);
		}

		printf("\nSuccess to send the whole file!!!\n");

		printf("\n-----------------------------------------\n");
		printf("All things done!!! Afternoon tea times!!!\n");
		printf("-----------------------------------------\n\n");
}
/*
*  Function: upload_file (NOT COMPLETE)
*  -------------------
*
*/

void upload_file(void *recv_socket_fd, char* path)
{

	printf("--------------------------------------\n");
    printf("Boss said he wants to upload file!!!\n");
    printf("--------------------------------------\n\n");

	int len;
	int *c_socket_fd;
	struct message_s PUT_REPLY;
	char* buff;
	struct message_s FILE_DATA;
	unsigned int filesize;
	FILE* fp;
	char* file_buff;

	c_socket_fd = (int*)recv_socket_fd;


	/* send PUT_REPLY protocol*/

	set_protocol(&PUT_REPLY, 0xB3, HEADER_LENGTH);

	printf("Sending PUT_REPLY message to server...............\n");
	if ((len = send(*c_socket_fd, &PUT_REPLY, HEADER_LENGTH, 0)) == -1) {
			printf("**********ERROR MESSAGES**********\n\n");
            printf("Fail to recv PUT_REPLY message from server\n");
            printf("\n**********************************\n");		
			exit(0);
	}
	else {
		printf("Success to send PUT_REPLY protocol from server!!!\n\n");
		// printf("DEBUG: Length of message : %d\n", len);
	}

	/*recv FILE_DATA protocol*/

	buff = (char*)malloc(sizeof(char)*BLOCK);
	memset(buff, '\0', sizeof(char)*BLOCK);

	printf("Receiving message from server...............\n");
	if ((len = recv(*c_socket_fd, buff, BLOCK, 0)) == -1) {
		printf("**********ERROR MESSAGES**********\n\n");
		printf("Fail to recv FILE_DATA message from server\n");
		printf("\n**********************************\n");
		exit(0);
	}
	else {
		printf("Success to receive FILE_DATA from server!!!\n\n");
	}

	memcpy(&FILE_DATA, buff, HEADER_LENGTH);
	int return_value;
	
	return_value = get_protocol_type(FILE_DATA, len);
	// printf("DEBUF: return value: %d\n", return_value);
	if (return_value == FILE_DATA_PROTOCOL) {

		filesize = ntohl(FILE_DATA.length) - HEADER_LENGTH;
		char* full_request_file_path;
		full_request_file_path = (char*)malloc(strlen(FILE_DIR) + strlen(path) + 1);
		strcpy(full_request_file_path, FILE_DIR);
		strcat(full_request_file_path, path);
		// printf("full path :%s\n", full_request_file_path);
		
		fp = fopen(full_request_file_path, "wb");

		file_buff = (char*)malloc(sizeof(char) * filesize);

		// printf("DEBUG: CHECKPOINT 1\n");
    	printf("Receiving file from server...............\n");
		if ((len = recvn(*c_socket_fd, file_buff, filesize)) == -1) {
			printf("\n**********ERROR MESSAGES**********\n\n");
			printf("Fail to receive file from client!!!\n\n");
			printf("\n**********************************\n");
			exit(0);
		}
		else {
			
		}
		// printf("DEBUG: CHECKPOINT 2\n");

		// printf("DEBUG: len of the chunks: %d\n", len);
		// printf("DEBUG: chunks information: %s\n", file_buff);
		printf("Processing...............\n");
    	printf("Please wait..........\n\n");

		fwrite(file_buff, 1, len, fp);

		fclose(fp);

		// printf("DEBUG: CHECKPOINT 3\n");
		
		printf("Success to get the file from client!!!\n\n");
		
		printf("\n-----------------------------------------\n");
		printf("All things done!!! Afternoon tea times!!!\n");
		printf("-----------------------------------------\n\n");
		
	}
	else {
		exit(0);
		//printf("FAIL!!!\n");
	}

	// printf("HERE?\n");




}

/*
*  Function pointer: client_activity
*  -------------------
*
*  recv_socket_fd: a void pointer pointing to the address of client socket.
*
*  returns: nothing
*
*  description: standard function for each thread in the program.
*
*  Warning: Use array instead of pointer in this threading function. Allocating ANY
*           memory with malloc() and calloc() into ANY variable would cause strange
*           behavior to function list_file()!!!
*           Reasons: unknown, affect by UFO I guess.
*
*  ERROR & TODO: The client program won't terminate even the server thread is terminated by pthread_exit()
*/

void *client_activity(void *recv_socket_fd)
{
	char buffer[BLOCK];
	unsigned int len;
	int *c_socket_fd;
	struct message_s REQUEST;

	c_socket_fd = (int*)recv_socket_fd;

    printf("Receiving protocol message from client...............\n");
	if ((len = recv(*c_socket_fd, buffer, BLOCK, 0)) == -1) {
		printf("**********ERROR MESSAGES**********\n\n");
		printf("Fail to receive message from client!!!\n");
		printf("error: %s (Errno:%d)\n", strerror(errno), errno);
		printf("\n**********************************\n");
		exit(0);
	}
	else {
		printf("Success to receive protocol message from client!!!\n\n");
	}

	// printf("DEBUG: Message recieved from threadID: %ld with size: %d\n\n", (unsigned long int)pthread_self(), len);
	// printf("DEBUG: Request.protocol: %s\n", REQUEST.protocol);
	// printf("DEBUG: Request.type: %c\n", REQUEST.type);
	// printf("DEBUG: Request.length: %d\n", REQUEST.length);

	memcpy(&REQUEST, buffer, HEADER_LENGTH); // extract protocol from message
	
    printf("Checking protocol...............\n");
	if (get_protocol_type(REQUEST, len) == LIST_REQUEST_PROTOCOL) {
        printf("Success to identify LIST_REQUEST protocol from message!!!\n\n");
		list_file(c_socket_fd);
	}
	else if (get_protocol_type(REQUEST, len) == GET_REQUEST_PROTOCOL) {
        printf("Success to identify GET_REQUEST protocol from message!!!\n\n");
		download_file(c_socket_fd, &buffer[HEADER_LENGTH]);
	}
	else if (get_protocol_type(REQUEST, len) == PUT_REQUEST_PROTOCOL) {
        printf("Success to identify PUT_REQUEST protocol from message!!!\n\n");
		upload_file(c_socket_fd, &buffer[HEADER_LENGTH]);
	}
	else if (get_protocol_type(REQUEST, len) == FILE_DATA_PROTOCOL) {
        printf("Success to identify FILE_DATA protocol from message!!!\n\n");
	}
	else {
        printf("\nClient left!!!\n\n");
	}

	pthread_exit(NULL);
}

void main_loop(unsigned short port)
{
	int s_socket_fd;
	int c_socket_fd;
	int c_addr_len;
	struct sockaddr_in server_address;
	struct sockaddr_in c_addr;
	pthread_t connection_thread;

	int val;

    printf("Creating socket for server...............\n");
	if ((s_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("\n**********ERROR MESSAGES**********\n\n");
        printf("Fail to create socket for server!!!\n");
        printf("\n**********************************\n");
	}
	else {
		printf("Success to create socket for server!!!\n\n");
	}

	if(setsockopt(s_socket_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		perror("Fail to reuse socket\n");
		exit(1);
	}


	/* bind() */

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);server_address.sin_port = htons(port);

    printf("Binding socket for server...............\n");
	if (bind(s_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		printf("\n**********ERROR MESSAGES**********\n\n");
		printf("Fail to bind socket to server!!!\n");
		printf("\n**********************************\n");
	}
	else {
		printf("Success to bind socket to server!!!\n\n");
	}

	/* listen() */

	printf("Setting to ready for listening socket...............\n");
	if (listen(s_socket_fd, 1024) == -1) {
		printf("\n**********ERROR MESSAGES**********\n\n");
		printf("Fail to listening socket!!!\n");
		printf("\n**********************************\n");
		close(s_socket_fd);
		exit(0);
	}
	else {
		printf("Success to setup to listen socket!!!\n\n");
	}

	/* accept() */
	printf("Ready for accepting sockets from clients...............\n");
	c_addr_len = sizeof(struct sockaddr_in);
	while (1)
	{
		if ((c_socket_fd = accept(s_socket_fd, (struct sockaddr *)&c_addr, &c_addr_len)) == -1) {
			printf("\n**********ERROR MESSAGES**********\n\n");
			printf("\nFail to accept socket from client!!!\n");
			printf("\n**********************************\n");
			exit(0);
		}
		else {
			printf("\nEstablish connection from client => PORT %d\n\n", ntohs(c_addr.sin_port));
		}

		/* operations after establishing the connection. */
		if (pthread_create(&connection_thread, NULL, client_activity, &c_socket_fd)) {
			printf("\n**********ERROR MESSAGES**********\n\n");
			fprintf(stderr, "\nFail to create thread for client operation!!!\n");
			printf("\n**********************************\n");
			exit(0);
		}
	}
	close(s_socket_fd);
}

int main(int argc, char **argv)
{
	printf("\n");
	if (argc != 2) {
		printf("\n**********ERROR MESSAGES**********\n\n");
		printf("Invalid command to start the server\nUsage: /myftpserver [PORT_NUMBER]\n");
		printf("\n**********************************\n");
	}
	else {
		main_loop(atoi(argv[1])); // convert the port number from string to integer and starts the program
	}

	return 0;
}