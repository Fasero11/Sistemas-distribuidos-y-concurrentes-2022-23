// Gonzalo Vega Pérez - 2022

#include "proxy_subscriber.h"
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
    struct message message;
    message.action = REGISTER_SUBSCRIBER;
    strcpy(message.topic, topic);

    client_socket = socket_create();

    socket_connect(client_socket);

    DEBUG_PRINTF("action: %d, topic: %s\n",message.action, message.topic);  
    if (send(client_socket, (void *)&message, sizeof(message), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }
    DEBUG_PRINTF("Message sent\n"); 

}