CC = gcc
LIB = -lpthread -lisal

all: 
	${CC} -o myftpserver myftpserver.c myftp.c ${LIB}
	${CC} -o myftpclient myftpclient.c myftp.c ${LIB}

clean:
	rm myftpserver
	rm myftpclient	
