// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void *receive(void *ptr){
    int *expected_lamport = (int*)ptr;

    // Recibe valor lamport = 1
    DEBUG_PRINTF("P2 recieving 1\n");
    socket_receive(1, expected_lamport[0]);

    // Recibe valor lamport = 1
    DEBUG_PRINTF("P2 recieving 1\n");
    socket_receive(1, expected_lamport[1]);

    // Envía valor lamport = 3
    DEBUG_PRINTF("P2 send shutdown_now 1\n");
    notify_shutdown_now();

    // Recibe valor lamport = 5
    DEBUG_PRINTF("P2 recieving 1\n");
    socket_receive(1, expected_lamport[2]);

    // Envía valor lamport = 7
    DEBUG_PRINTF("P2 send shutdown_now 1\n");
    notify_shutdown_now();

    // Recibe valor lamport = 9
    DEBUG_PRINTF("P2 recieving 1\n");
    socket_receive(1, expected_lamport[3]);
    
    pthread_exit(NULL);
};

// SERVIDOR: P2
// Recibe de P1 y P3 READY_TO_SHUTDOWN
// Envía SHUTDOWN_NOW
// Recibe SHUTDOWN_ACK de P1
// Envía SHUTDOWN_NOW
// Recibe SHUTDOWN_ACK de P3
int main(int argc, char **args) {
    // Establece los valores del reloj de lamport que espera recibir en orden.
    int expected_lamport_1[4] = {1,1,5,9};

    char* ip;
    unsigned int port;

    if (argc != 3){
        printf("usage: P1 IP PORT\n");
        exit(1);
    };

    ip = args[1];
    port = atoi(args[2]);

    set_name("P2");

    DEBUG_PRINTF("P2 create socket\n");
    socket_create();

    DEBUG_PRINTF("P2 set IP and Port\n");
    set_ip_port(ip,port);

    DEBUG_PRINTF("P2 binding\n");
    socket_bind();

    DEBUG_PRINTF("P2 listening\n");
    socket_listen();

    DEBUG_PRINTF("P2 accepting\n");
    socket_accept();

    DEBUG_PRINTF("New P2 Thread!\n");  
    pthread_t thread1;
    if (pthread_create(&thread1, NULL, receive, (void *)&expected_lamport_1) < 0){
        warnx("Error while creating Thread\n");
        socket_close(1);
        exit(1);
    }

    if (pthread_join(thread1, NULL) < 0) {
        warnx("Error while joining Thread\n");
        socket_close(1);
        exit(1);
    };

    printf("Los clientes fueron correctamente apagados en t(lamport) = %d\n",get_clock_lamport());
    socket_close(1);
    return(0);
}