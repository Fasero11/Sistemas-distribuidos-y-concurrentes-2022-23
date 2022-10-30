// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void *receive(void *ptr){
    // Recibe valor lamport = 7
    DEBUG_PRINTF("P3 recieving 1\n");
    socket_receive(0, 7);

    pthread_exit(NULL);
};

// CLIENTE: P3
// Envía a P2 READY_TO_SHUTDOWN
// Recibe de P2 SHUTDOWN_NOW
// Envía a P2 SHUTDOWN_ACK
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

    // Envía valor lamport = 1
    DEBUG_PRINTF("P3 send ready_shutdown 1\n");
    notify_ready_shutdown();

    pthread_t thread;

    if (pthread_create(&thread, NULL, receive, (void *)NULL) < 0){
        warnx("Error while creating Thread\n");
        socket_close(0);
        exit(1);
    };

    if (pthread_join(thread, NULL) < 0) {
        warnx("Error while joining Thread\n");
        socket_close(0);
        exit(1);
    };

    // Envía valor lamport = 9
    DEBUG_PRINTF("P3 send shutdown_ack 1\n");
    notify_shutdown_ack();

    socket_close(0);
    return 0;
}