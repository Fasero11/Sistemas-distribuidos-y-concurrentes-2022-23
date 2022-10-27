CFLAGS = -Wall -Wshadow -pthread
	
all: proxy P1 P2 P3 

proxy: proxy.c
	gcc -c proxy.c $(CFLAGS) $(DEBUG)
	
P1: P1.c
	gcc P1.c proxy.o -o P1  $(CFLAGS) $(DEBUG)
	
P2: P2.c
	gcc P2.c proxy.o -o P2 $(CFLAGS) $(DEBUG)

P3: P3.c
	gcc P3.c proxy.o -o P3 $(CFLAGS) $(DEBUG)
