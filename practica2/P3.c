// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Send P2 READY_TO_SHUTDOWN
// Recieve from P2 SHUTDOWN_NOW
// Send P2 SHUTDOWN_ACK
int main(int argc, char **args) {
    set_name("P3");

    DEBUG_PRINTF("P3 create socket\n");
    socket_create();

    DEBUG_PRINTF("P3 set IP and Port\n");
    set_ip_port("127.0.0.3",8080);
    
    DEBUG_PRINTF("P3 connecting...\n");
    socket_connect();

    notify_ready_shutdown();

    socket_close();
    return 0;
}