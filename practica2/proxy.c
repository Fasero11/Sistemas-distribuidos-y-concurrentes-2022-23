// Gonzalo Vega Pérez - 2022

#include "proxy.h"

struct message msg;
struct sockaddr_in addr, client_addr;
char proc_name[2];
int socket_, client_socket_;

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[2]){
    strcpy(proc_name,name);
};
    
// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port){
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    DEBUG_PRINTF("%s: IP set: %s | Port set: %d \n",proc_name ,ip, port);
};

// Obtiene el valor del reloj de lamport.
// Utilízalo cada vez que necesites consultar el tiempo.
// Esta función NO puede realizar ningún tiempo de comunicación (sockets)
int get_clock_lamport(){
    return 0;
};

// Notifica que está listo para realizar el apagado (READY_TO_SHUTDOWN)
void notify_ready_shutdown(){

    strcpy(msg.origin, proc_name);
    msg.action = READY_TO_SHUTDOWN;
    msg.clock_lamport = get_clock_lamport() + 1;

    if (send(socket_, (void *)&msg, sizeof(msg), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(socket_);
        exit(1);
    }
    DEBUG_PRINTF("%s: Notify Ready_Shutdown \n",proc_name);
};

// Notifica que va a realizar el shutdown correctamente (SHUTDOWN_ACK)
void notify_shutdown_ack();

void socket_create(){
    setbuf(stdout, NULL);
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0){
        warnx("%s: Error creating socket. %s\n",proc_name ,strerror(errno));
        exit(1); 
    }
    DEBUG_PRINTF("%s: Socket successfully created...\n",proc_name );
};

void socket_connect(){
    while (connect(socket_, (struct sockaddr *)&addr, sizeof(addr)) != 0){
        //DEBUG_PRINTF("%s: Server not found\n",proc_name );
    }
    DEBUG_PRINTF ("%s: Connected to server...\n",proc_name );
};

void socket_bind(){
    if (bind(socket_, (struct sockaddr*)&addr, 
    sizeof(addr)) < 0){
        warnx("%s: bind() failed. %s\n",proc_name , strerror(errno));
        close(socket_);
        exit(1); 
    }
    DEBUG_PRINTF("%s: Socket successfully binded...\n",proc_name );
    DEBUG_PRINTF("%s: Binded to port: %d\n",proc_name , addr.sin_port);

};

void socket_listen(){
    DEBUG_PRINTF("%s: Socket listening...\n",proc_name );
    if (listen(socket_, 5) < 0){
        warnx("%s: listen() failed. %s\n",proc_name , strerror(errno));
        close(socket_);
        exit(1);        
    }
};

void socket_accept(){
    socklen_t addr_size = sizeof(client_addr);
    client_socket_ = accept(socket_, (struct sockaddr*)&client_addr, 
    &addr_size);

    if (client_socket_ < 0){
        warnx("%s: accept() failed. %s\n",proc_name , strerror(errno));
        close(client_socket_);
        close(socket_);
        exit(1);
    }
    DEBUG_PRINTF("%s: Client Connected!. client_socket_: %d\n",proc_name ,client_socket_);
};

void socket_recieve(){
    if (recv(client_socket_, (void *)&msg, sizeof(msg), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        close(socket_);
        close(client_socket_);
        exit(1);
    }
    printf("+++ %s, %d, %d\n", msg.origin, msg.action ,msg.clock_lamport);
}

void socket_close(){
    close(socket_);
};