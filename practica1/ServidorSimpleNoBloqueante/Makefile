all: server client
CFLAGS = -Wall -Wshadow

server: server.c 
	gcc -o server server.c $(CFLAGS) $(DEBUG)
	
client: client.c
	gcc -o client client.c $(CFLAGS) $(DEBUG)
