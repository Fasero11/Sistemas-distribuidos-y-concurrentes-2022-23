// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Recieve from P1 & P3 READY_TO_SHUTDOWN
// Send P1 SHUTDOWN_NOW
// Recieve SHUTDOWN_ACK from P1
// Send P3 SHUTDOWN_NOW
// Recieve SHUTDOWN_ACK from P3
int main(int argc, char **args) {
    set_name("P2");

    DEBUG_PRINTF("P2 create socket\n");
    socket_create();

    DEBUG_PRINTF("P2 set IP and Port\n");
    set_ip_port("0.0.0.0",8080);

    DEBUG_PRINTF("P2 binding\n");
    socket_bind();

    DEBUG_PRINTF("P2 listening\n");
    socket_listen();

    DEBUG_PRINTF("P2 accepting\n");
    socket_accept();

    DEBUG_PRINTF("P2 recieving 1\n");
    socket_recieve(1);

    DEBUG_PRINTF("P2 send shutdown_now 1\n");
    notify_shutdown_now();

    DEBUG_PRINTF("P2 recieving 2\n");
    socket_recieve(1);

    DEBUG_PRINTF("P2 send shutdown_now 2\n");
    notify_shutdown_now();
    
    DEBUG_PRINTF("P2 recieving 3\n");
    socket_recieve(1);

    socket_close();
    return 0;
}