// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void *receive(void *ptr){
    // Recieve Lamport = 7
    DEBUG_PRINTF("P3 recieving 1\n");
    socket_receive(0, 7);

    pthread_exit(NULL);
};

// Send P2 READY_TO_SHUTDOWN
// Recieve from P2 SHUTDOWN_NOW
// Send P2 SHUTDOWN_ACK
int main(int argc, char **args) {
    char* ip;
    unsigned int port;

    if (argc != 3){
        printf("usage: P3 IP PORT\n");
        exit(1);
    };

    ip = args[1];
    port = atoi(args[2]);

    set_name("P3");

    DEBUG_PRINTF("P3 create socket\n");
    socket_create();

    DEBUG_PRINTF("P3 set IP and Port\n");
    set_ip_port(ip,port);
    
    DEBUG_PRINTF("P3 connecting...\n");
    socket_connect();

    // Send Lamport = 1
    DEBUG_PRINTF("P3 send ready_shutdown 1\n");
    notify_ready_shutdown();

    pthread_t thread;
    // Create Thread
    if (pthread_create(&thread, NULL, receive, (void *)NULL) < 0){
        warnx("Error while creating Thread\n");
        socket_close();
        exit(1);
    };

    if (pthread_join(thread, NULL) < 0) {
        warnx("Error while joining Thread\n");
        socket_close();
        exit(1);
    };

    // Send Lamport = 9
    DEBUG_PRINTF("P3 send shutdown_ack 1\n");
    notify_shutdown_ack();

    socket_close();
    return 0;
}