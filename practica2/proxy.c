// Gonzalo Vega Pérez - 2022

#include "proxy.h"

struct message msg;
struct sockaddr_in addr;
struct sockaddr_in client_addr;

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[2]){
    printf("change name 2\n");
};
    
// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port){
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    DEBUG_PRINTF("IP set: %s | Port set: %d \n",ip, port);
};

// Obtiene el valor del reloj de lamport.
// Utilízalo cada vez que necesites consultar el tiempo.
// Esta función NO puede realizar ningún tiempo de comunicación (sockets)
int get_clock_lamport();

// Notifica que está listo para realizar el apagado (READY_TO_SHUTDOWN)
void notify_ready_shutdown();

// Notifica que va a realizar el shutdown correctamente (SHUTDOWN_ACK)
void notify_shutdown_ack();

int create_socket(){
    setbuf(stdout, NULL);
    int sckt = socket(AF_INET, SOCK_STREAM, 0);
    if (sckt < 0){
        warnx("Error creating socket. %s\n",strerror(errno));
        exit(1); 
    }
    DEBUG_PRINTF("Socket successfully created...\n");
    return sckt;
};

void socket_connect(int socket){
    while (connect(socket, (struct sockaddr *)&addr, sizeof(addr)) != 0){
        //DEBUG_PRINTF("Server not found\n");
    }
    DEBUG_PRINTF ("Connected to server...\n");
};

void bind_socket(int socket){
    if (bind(socket, (struct sockaddr*)&addr, 
    sizeof(addr)) < 0){
        warnx("bind() failed. %s\n", strerror(errno));
        close(socket);
        exit(1); 
    }
    DEBUG_PRINTF("Socket successfully binded...\n");
    DEBUG_PRINTF("Binded to port: %d\n", addr.sin_port);

};

void listen_socket(int socket){
    DEBUG_PRINTF("Socket listening...\n");
    if (listen(socket, 5) < 0){
        warnx("listen() failed. %s\n", strerror(errno));
        close(socket);
        exit(1);        
    }
};

int accept_socket(int socket){
    socklen_t addr_size = sizeof(client_addr);
    int client_socket = accept(socket, (struct sockaddr*)&client_addr, 
    &addr_size);

    if (client_socket < 0){
        warnx("accept() failed. %s\n", strerror(errno));
        close(client_socket);
        close(socket);
        exit(1);
    }
    DEBUG_PRINTF("Client Connected!. client_socket: %d\n",client_socket);
    return client_socket;
};