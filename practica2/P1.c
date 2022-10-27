// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Send P2 READY_TO_SHUTDOWN
// Recieve from P2 SHUTDOWN_NOW
// Send P2 SHUTDOWN_ACK
struct message msg;
int main(int argc, char **args) {
    set_name("P1");
    return 0;
};