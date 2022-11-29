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

/* FUNCIONES COMPARTIDAS */ 

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[6]);

// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port);


/* FUNCIONES DEL CLIENTE */ 

// Se conecta al servidor, intercambia mensajes y desconecta.
void *talk_2_server(void *ptr);

// Recibe y devuelve un mensaje tipo response
struct response receive_response(int socket_);

// Establece conexión con el servidor.
void socket_connect(int socket_);


/* FUNCIONES DEL SERVIDOR */ 

// Inicializa el servidor.
void init_server(char* ip, int port, char* priority_, int ratio_);

// Acepta una conexión y guarda en "client_sockets" el socket del cliente
// en una posición específica.
void init_server_thread(int *thread_info);

// Intercambio de mensajes con el cliente y finalización de la conexión.
void *talk_2_client(void *ptr);

// Sobrescribe el contador actual en el fichero "server_output.txt"
void write_output();

// Lee el fichero server_output.txt y establece el contador al valor leido.
// Si no existe el fichero, se crea.
void read_output();

// Envía el mensaje response a través del socket indicado.
void send_response(struct response response, int socket);

// Ejecuta la tarea indicada en request.
// Devuelve un mensaje tipo response.
struct response do_request(struct request request);

// Establece el ratio entre clientes con y sin prioridad.
void set_ratio(int ratio_);

// Establece la prioridad del servidor READ/WRITE
void set_priority(char *priority);

// Establece el valor del contador.
void set_counter(int counter_);

// Suma al número actual de threads el valor indicado.
void set_current_threads(int value);

// Devuelve el número actual de threads activos
int get_current_threads();

// Devuelve la primera posición del array "client_sockets" que no está
// siendo utilizada por ningún thread.
int get_free_fd();

// Guarda el value en la posición id de "client_sockets"
void set_value_fd (int id, int value);

// Crea un socket para TCP
int socket_create();

// Hace bind con la IP y puerto establecidos en addr.
void socket_bind(int socket_);

// Marca el socket creado como pasivo.
void socket_listen(int socket_);

// Acepta la conexión entrante. (Solo servidor)
int socket_accept(int socket_);

// Recibe y devuelve un mensaje tipo request
struct request receive_request(int client_socket_);

#endif // PROXY_H
