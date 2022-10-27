// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Send P2 READY_TO_SHUTDOWN
// Recieve from P2 SHUTDOWN_NOW
// Send P2 SHUTDOWN_ACK
int main(int argc, char **args) {
    // socket
    // connect
    // send
    // recieve
    DEBUG_PRINTF("P1 create socket\n");
    int  socket = create_socket();

    DEBUG_PRINTF("P1 set IP and Port\n");
    set_ip_port("127.0.0.1",8080);

    DEBUG_PRINTF("P1 connecting..\n");
    socket_connect(socket);

    close(socket);
    return 0;
};