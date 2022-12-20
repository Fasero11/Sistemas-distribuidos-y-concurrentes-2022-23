// Gonzalo Vega Pérez - 2022

#include "proxy_publisher.h"
struct sockaddr_in addr;
char proc_name[32];
int client_socket;
int mode;

void set_name (char name[6]){
    strcpy(proc_name,name);
};

void set_ip_port (char* ip, unsigned int port){
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    DEBUG_PRINTF("%s: IP set: %s | Port set: %d \n",proc_name ,ip, port);
};

int socket_create(){
    setbuf(stdout, NULL);
    int socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0){
        warnx("%s: Error creating socket. %s\n",proc_name ,strerror(errno));
        exit(1); 
    }
    DEBUG_PRINTF("%s: Socket successfully created...\n",proc_name );

    return socket_;
};

void socket_connect(int socket_){
    if (connect(socket_, (struct sockaddr *)&addr, sizeof(addr)) != 0){
        DEBUG_PRINTF("Error connecting\n");
        exit(EXIT_FAILURE);
    }
    DEBUG_PRINTF ("%s: Connected to server...\n",proc_name );
};

void talk_to_server(char *topic){

    struct timespec time;
    long time_seconds;
    long time_nanoseconds;

    struct response response;

    struct message message;
    message.action = REGISTER_PUBLISHER;
    strcpy(message.topic, topic);

    client_socket = socket_create();

    socket_connect(client_socket);

    clock_gettime(CLOCK_MONOTONIC, &time);
    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;
    printf("[%ld.%ld] Publisher conectado con broker correctamente\n",
    time_seconds, time_nanoseconds);


    DEBUG_PRINTF("action: %d, topic: %s\n",message.action, message.topic);  
    if (send(client_socket, (void *)&message, sizeof(message), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    if (recv(client_socket, (void *)&response, sizeof(response), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        exit(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &time);
    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;
    if (response.response_status == OK){
        printf("[%ld.%ld] Registrado correctamente con ID: %d para topic %s\n",
        time_seconds, time_nanoseconds, response.id, topic);    
    } else {
        printf("[%ld.%ld] Error al hacer el registro: error=%d\n",
        time_seconds, time_nanoseconds, response.response_status);       
    }

    DEBUG_PRINTF("Message sent\n"); 

}