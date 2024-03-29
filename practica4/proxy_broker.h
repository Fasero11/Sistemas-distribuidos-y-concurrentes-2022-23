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
#include <semaphore.h>

#define MAX_TOPICS 10
#define MAX_PUBLISHERS 100
#define MAX_SUBSCRIBERS 900

#define SECUENCIAL 0
#define PARALELO 1
#define JUSTO 2

struct publisher {
    char topic[100];
    int id;             // Real id
    int id_in_topic;    // id inside the topic's publishers array
    int fd;             // File descriptor for this client
};

struct subscriber {
    char topic[100];
    int id;             // Real id
    int id_in_topic;    // id inside the topic's subscribers array
    int fd;             // File descriptor for this client
};

struct topic {
    char name[100];
    char data[1024];
    int num_subscribers;
    int num_publishers;
    struct subscriber subscribers[MAX_SUBSCRIBERS];
    struct publisher publishers[MAX_PUBLISHERS];
};

enum operations {
REGISTER_PUBLISHER = 0,
UNREGISTER_PUBLISHER,
REGISTER_SUBSCRIBER,
UNREGISTER_SUBSCRIBER,
PUBLISH_DATA
};

struct publish {
    struct timespec time_generated_data;
    char data[100];
};

struct message {
    enum operations action;
    char topic[100];
    // Solo utilizado en mensajes de UNREGISTER
    int id;
    // Solo utilizado en mensajes PUBLISH_DATA
    struct publish data;
};

struct send_message {
    struct message message;
    int fd;
};

enum status {
    ERROR = 0,
    LIMIT,
    OK
};

struct response {
    enum status response_status;
    int id;
};

void send_sequential(struct message message);

void* send_parallel(void *ptr);

void* send_just(void *ptr);

void publish_msg(struct message message);

void shift_client_list(struct topic *topic, int action, int id);

int get_topic_id(char *topic);

void unregister(int action, char *topic, int id);

void talk_to_publisher(int client_socket_);

void talk_to_subscriber(int client_socket_);

void sighandler(int signum);

int create_topic(char *name);

void init_server_thread(int *thread_info);

void *talk_to_client(void *ptr);

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[6]);

// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port);

// Recibe y devuelve un mensaje tipo response
struct response receive_response(int socket_);

// Establece conexión con el servidor.
void socket_connect(int socket_);

// Inicializa el servidor.
void init_broker(char* ip, int port, char* mode);

// Crea un socket para TCP
int socket_create();

// Hace bind con la IP y puerto establecidos en addr.
void socket_bind(int socket_);

// Marca el socket creado como pasivo.
void socket_listen(int socket_);

// Acepta la conexión entrante. (Solo servidor)
int socket_accept(int socket_);


#endif // PROXY_H
