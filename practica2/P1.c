// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// CLIENTE: P1
// Envía a P2 READY_TO_SHUTDOWN. SEND LAMPORT 1
// Recibe de P2 SHUTDOWN_NOW.    RECV LAMPORT 3
// Envía a P2 SHUTDOWN_ACK       SEND LAMPORT 5
int main(int argc, char **args) {
    char* ip;
    unsigned int port;

    if (argc != 3){
        printf("usage: P1 IP PORT\n");
        exit(1);
    };

    ip = args[1];
    port = atoi(args[2]);

    set_name("P1");
    set_ip_port(ip,port);
    init_client();

    pthread_t thread;

    if (pthread_create(&thread, NULL, client_receive, (void *)NULL) < 0){
        warnx("Error while creating Thread\n");
        close_client();
        exit(1);
    };

    if (pthread_join(thread, NULL) < 0) {
        warnx("Error while joining Thread\n");
        close_client();
        exit(1);
    };

    close_client();
    return 0;
};