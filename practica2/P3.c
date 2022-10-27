// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Send P2 READY_TO_SHUTDOWN
// Recieve from P2 SHUTDOWN_NOW
// Send P2 SHUTDOWN_ACK
int main(int argc, char **args) {
    DEBUG_PRINTF("P3 create socket\n");
    int socket = create_socket();

    DEBUG_PRINTF("P3 set IP and Port\n");
    set_ip_port("127.0.0.3",8080);
    
    DEBUG_PRINTF("P3 connecting...\n");
    socket_connect(socket);

    close(socket);
    return 0;
}