CFLAGS = -Wall -Wshadow -pthread
	
all: broker subscriber

proxy_broker: proxy_broker.c 
	gcc -c proxy_broker.c $(CFLAGS) $(DEBUG)

#proxy_publisher: proxy_publisher.c 
#	gcc -c proxy_publisher.c $(CFLAGS) $(DEBUG)

proxy_subscriber: proxy_subscriber.c 
	gcc -c proxy_subscriber.c $(CFLAGS) $(DEBUG)

broker: broker.c proxy_broker.c
	gcc broker.c proxy_broker.c -o broker $(CFLAGS) $(DEBUG)
	
#publisher: publisher.c proxy_publisher.c
#	gcc publisher.c proxy_publisher.c -o publisher $(CFLAGS) $(DEBUG)

subscriber: subscriber.c proxy_subscriber.c
	gcc subscriber.c proxy_subscriber.c -o subscriber $(CFLAGS) $(DEBUG)


	