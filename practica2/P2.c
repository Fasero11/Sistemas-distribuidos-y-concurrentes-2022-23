// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// SERVIDOR: P2
// Recibe de P1 y P3 READY_TO_SHUTDOWN  RECV LAMPORT 1
// Envía SHUTDOWN_NOW                   SEND LAMPORT 3
// Recibe SHUTDOWN_ACK de P1            RECV LAMPORT 5
// Envía SHUTDOWN_NOW                   SEND LAMPORT 7
// Recibe SHUTDOWN_ACK de P3            RECV LAMPORT 9
int main(int argc, char **args) {
    signal(SIGINT, signal_handler);
    // Establece los valores del reloj de lamport que espera recibir en orden.

    char* ip;
    unsigned int port;

    if (argc != 3){
        printf("usage: P1 IP PORT\n");
        exit(1);
    };

    ip = args[1];
    port = atoi(args[2]);

    set_name("P2");
    set_ip_port(ip,port);
    init_server();

    DEBUG_PRINTF("New P2 Thread!\n");  
    pthread_t thread1;
    if (pthread_create(&thread1, NULL, server_receive, (void *)NULL) < 0){
        warnx("Error while creating Thread\n");
        close_server();
        exit(1);
    }

    if (pthread_join(thread1, NULL) < 0) {
        warnx("Error while joining Thread\n");
        close_server();
        exit(1);
    };

    printf("Los clientes fueron correctamente apagados en t(lamport) = %d\n",get_clock_lamport());
    close_server();
    return(0);
}