// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Send P2 READY_TO_SHUTDOWN
// Recieve from P2 SHUTDOWN_NOW
// Send P2 SHUTDOWN_ACK
int main(int argc, char **args) {
    set_name("P1");

    DEBUG_PRINTF("P1 create socket\n");
    socket_create();

    DEBUG_PRINTF("P1 set IP and Port\n");
    set_ip_port("127.0.0.1",8080);

    DEBUG_PRINTF("P1 connecting...\n");
    socket_connect();

    // Send Lamport = 1
    DEBUG_PRINTF("P1 send ready_shutdown 1\n");
    notify_ready_shutdown();

    // Recieve Lamport = 3
    DEBUG_PRINTF("P1 recieving 1\n");
    socket_recieve(0, 3);

    // Send Lamport = 5
    DEBUG_PRINTF("P1 send shutdown_ack 1\n");
    notify_shutdown_ack();

    socket_close();
    return 0;
};