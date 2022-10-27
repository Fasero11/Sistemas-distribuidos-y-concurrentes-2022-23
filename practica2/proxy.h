// Gonzalo Vega Pérez - 2022

#ifndef PROXY_H
#define PROXY_H

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

enum operations {
READY_TO_SHUTDOWN = 0,
SHUTDOWN_NOW,
SHUTDOWN_ACK
};

struct message {
char origin[20];
enum operations action;
unsigned int clock_lamport;
};

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[2]);

// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port);

// Obtiene el valor del reloj de lamport.
// Utilízalo cada vez que necesites consultar el tiempo.
// Esta función NO puede realizar ningún tiempo de comunicación (sockets)
int get_clock_lamport();

// Notifica que está listo para realizar el apagado (READY_TO_SHUTDOWN)
void notify_ready_shutdown();

// Notifica que va a realizar el shutdown correctamente (SHUTDOWN_ACK)
void notify_shutdown_ack();

void notify_shutdown_now();

void socket_create();

void socket_connect();

void socket_bind();

void socket_listen();

void socket_accept();

void socket_recieve(int is_server, int expected_lamport);

void socket_close();

void get_action(char **action);
#endif // PROXY_H
