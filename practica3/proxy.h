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

// Custom structure for client threads
struct client_threads{
    char *mode;          // Mode: writer / reader
    int thread_id;      // ID of the thread  
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
void socket_create();

// Hace un bucle hasta que consigue establecer conexión.
void socket_connect();

// Hace bind con la IP y puerto establecidos en addr.
void socket_bind();

// Marca el socket creado como pasivo.
void socket_listen();

void socket_accept();

struct request receive_request();

struct response receive_response();

void send_response(struct response response);

struct response do_request(struct request request);

void *talk_2_server(void *ptr);

void close_server();

void close_client();

void write_output();

#endif // PROXY_H
