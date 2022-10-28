// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void *receive(void *ptr){

    int *expected_lamport = (int*)ptr;
    DEBUG_PRINTF("P2 recieving 1\n");
    socket_receive(1, expected_lamport[0]);

    // DEBUG_PRINTF("P2 recieving 1\n");
    // socket_receive(1, expected_lamport[1]);

    // Send Lamport = 7
    DEBUG_PRINTF("P2 send shutdown_now 1\n");
    notify_shutdown_now();

    DEBUG_PRINTF("P2 recieving 1\n");
    socket_receive(1, expected_lamport[2]);

    // Send Lamport = 7
    DEBUG_PRINTF("P2 send shutdown_now 1\n");
    notify_shutdown_now();

    DEBUG_PRINTF("P2 recieving 1\n");
    socket_receive(1, expected_lamport[3]);
    
    pthread_exit(NULL);
};

// Recieve from P1 & P3 READY_TO_SHUTDOWN
// Send P1 SHUTDOWN_NOW
// Recieve SHUTDOWN_ACK from P1
// Send P3 SHUTDOWN_NOW
// Recieve SHUTDOWN_ACK from P3
int main(int argc, char **args) {
    int expected_lamport_1[4] = {1,1,5,9};
    // int expected_lamport_2[2] = {1,9};
    set_name("P2");

    DEBUG_PRINTF("P2 create socket\n");
    socket_create();

    DEBUG_PRINTF("P2 set IP and Port\n");
    set_ip_port("0.0.0.0",8080);

    DEBUG_PRINTF("P2 binding\n");
    socket_bind();

    DEBUG_PRINTF("P2 listening\n");
    socket_listen();

    // Accept connection
    DEBUG_PRINTF("P2 accepting\n");
    socket_accept();

  
    DEBUG_PRINTF("New P2 Thread!\n");  
    pthread_t thread1;
   // Recieve Lamport = 1, 5
    if (pthread_create(&thread1, NULL, receive, (void *)&expected_lamport_1) < 0){
        warnx("Error while creating Thread\n");
        socket_close();
    }

//     DEBUG_PRINTF("New P2 Thread!\n");
//     pthread_t thread2;
//    // Recieve Lamport = 1, 9
//     if (pthread_create(&thread2, NULL, receive, (void *)&expected_lamport_2) < 0){
//         warnx("Error while creating Thread\n");
//         socket_close();
//     }

    if (pthread_join(thread1, NULL) < 0) {
        warnx("Error while joining Thread\n");
        socket_close();
        exit(1);
    };

    // if (pthread_join(thread2, NULL) < 0) {
    //     warnx("Error while joining Thread\n");
    //     socket_close();
    //     exit(1);
    // };

    printf("Los clientes fueron correctamente apagados en t(lamport) = %d\n",get_clock_lamport());
}