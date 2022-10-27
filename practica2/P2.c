// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Recieve from P1 & P3 READY_TO_SHUTDOWN
// Send P1 SHUTDOWN_NOW
// Recieve SHUTDOWN_ACK from P1
// Send P3 SHITDOWN_NOW
// Recieve SHUTDOWN_ACK from P3
int main(int argc, char **args) {

    DEBUG_PRINTF("P2 create socket\n");
    int  socket = create_socket();

    DEBUG_PRINTF("P2 set IP and Port\n");
    set_ip_port("0.0.0.0",8080);

    DEBUG_PRINTF("P2 binding\n");
    bind_socket(socket);

    DEBUG_PRINTF("P2 listening\n");
    listen_socket(socket);

    DEBUG_PRINTF("P2 accepting\n");
    int client_socket = accept_socket(socket);

    close(socket);
    return 0;
}