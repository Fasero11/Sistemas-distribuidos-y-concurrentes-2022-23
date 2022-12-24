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
#include <sys/select.h>
#include <signal.h>

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

void signal_handler(int signal);

void close_client();

void close_server();

void send_msg_to_client(int lamport);

void *client_receive(void *ptr);

void *server_receive(void *ptr);

void init_client();

void init_server();

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

// Notifica al cliente que puede apagarse (SHUTDOWN_NOW)
void notify_shutdown_now();

// Crea un socket para TCP
void socket_create();

// Hace un bucle hasta que consigue establecer conexión.
void socket_connect();

// Hace bind con la IP y puerto establecidos en addr.
void socket_bind();

// Marca el socket creado como pasivo.
void socket_listen();

// Acepta las dos conexiones de los clientes.
// No sale hasta que los dos clientes hayan sido aceptados.
void socket_accept();

// Escribe en action la acción equivalente al valor de enum operations de in_action
// READY_TO_SHUTDOWN = 0
// SHUTDOWN_NOW = 1
// SHUTDOWN_ACK = 2
void action_to_str(char **action, int in_action);

// Se encarga de hacer el recv() no bloqueante (timeout 0.5s).
// Devuelve 1 si se han recibido datos. 0 en caso contrario.
int do_receive_in_socket(int socket, struct message *recv_msg);

#endif // PROXY_H
