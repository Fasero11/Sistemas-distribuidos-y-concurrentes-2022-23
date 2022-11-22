CFLAGS = -Wall -Wshadow -pthread
	
all: client server

proxy: proxy.c 
	gcc -c proxy.c $(CFLAGS) $(DEBUG)

client: client.c proxy.c
	gcc client.c proxy.c -o client  $(CFLAGS) $(DEBUG)
	
server: server.c proxy.c
	gcc server.c proxy.c -o server $(CFLAGS) $(DEBUG)