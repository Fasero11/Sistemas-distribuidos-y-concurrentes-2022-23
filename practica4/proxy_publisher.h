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

enum status {
    ERROR = 0,
    LIMIT,
    OK
};

struct response {
    enum status response_status;
    int id;
};

void sighandler(int signum);

void talk_to_server(char* topic);

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[6]);

// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port);

// Establece conexión con el servidor.
void socket_connect(int socket_);

// Crea un socket para TCP
int socket_create();


#endif // PROXY_H
