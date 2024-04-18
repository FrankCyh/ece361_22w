all: message server client

message: message.c
	gcc -c message.c

server: server.c
	gcc -g -c server.c
	gcc message.o server.o -o server 

client: client.c
	gcc -g -c client.c
	gcc message.o client.o -lm -o client
