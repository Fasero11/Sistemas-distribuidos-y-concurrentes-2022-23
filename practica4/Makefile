CFLAGS = -Wall -Wshadow -pthread
	
all: broker subscriber publisher

proxy_broker: proxy_broker.c 
	gcc -c proxy_broker.c $(CFLAGS) $(DEBUG)

proxy_client: proxy_client.c 
	gcc -c proxy_client.c $(CFLAGS) $(DEBUG)

broker: broker.c proxy_broker.c
	gcc broker.c proxy_broker.c -o broker $(CFLAGS) $(DEBUG)
	
publisher: publisher.c proxy_client.c
	gcc publisher.c proxy_client.c -o publisher $(CFLAGS) $(DEBUG)

subscriber: subscriber.c proxy_client.c
	gcc subscriber.c proxy_client.c -o subscriber $(CFLAGS) $(DEBUG)


	