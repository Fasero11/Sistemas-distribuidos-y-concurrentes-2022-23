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
#include <getopt.h>

#define MAX_THREADS 250

// Custom structure for client threads
struct client_threads{
    char *mode;         // Mode: writer / reader
    int thread_id;      // ID of the thread  
    int socket;         // Client fd
};

enum operations {
WRITE = 0,
READ
};

struct request {
enum operations action;
unsigned int id;
};

struct response {
enum operations action;
unsigned int counter;
long waiting_time;
};

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[6]);

// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port);

// Crea un socket para TCP
int socket_create();

// Hace un bucle hasta que consigue establecer conexión.
void socket_connect(int socket_);

// Hace bind con la IP y puerto establecidos en addr.
void socket_bind(int socket_);

// Marca el socket creado como pasivo.
void socket_listen(int socket_);

int socket_accept(int socket_);

struct request receive_request(int client_socket_);

struct response receive_response(int socket_);

void send_response(struct response response, int client_socket_);

struct response do_request(struct request request);

void *talk_2_server(void *ptr);

void close_client_socket(int client_socket_);

void close_client(int socket_);

void write_output();

void *talk_2_client(void *ptr);

#endif // PROXY_H
