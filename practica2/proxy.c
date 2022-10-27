// Gonzalo Vega Pérez - 2022

#include "proxy.h"

struct message msg;
struct sockaddr_in addr, client_addr;
char proc_name[2];
int socket_, client_socket_;
int lamport_ = 0;

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
    return lamport_;
};

// Notifica que está listo para realizar el apagado (READY_TO_SHUTDOWN)
void notify_ready_shutdown(){

    strcpy(msg.origin, proc_name);
    msg.action = READY_TO_SHUTDOWN;
    lamport_ = get_clock_lamport() + 1;
    msg.clock_lamport = lamport_;

    if (send(socket_, (void *)&msg, sizeof(msg), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(socket_);
        exit(1);
    }
    printf("%s, %d, SEND, READY_TO_SHUTDOWN\n", proc_name, lamport_);
};

// Notifica que va a realizar el shutdown correctamente (SHUTDOWN_ACK)
void notify_shutdown_ack(){
    strcpy(msg.origin, proc_name);
    msg.action = SHUTDOWN_ACK;
    lamport_ = get_clock_lamport() + 1;
    msg.clock_lamport = lamport_;

    if (send(socket_, (void *)&msg, sizeof(msg), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(socket_);
        exit(1);
    }
    printf("%s, %d, SEND, SHUTDOWN_ACK\n", proc_name, lamport_);
};

void notify_shutdown_now(){
    strcpy(msg.origin, proc_name);
    msg.action = SHUTDOWN_NOW;
    lamport_ = get_clock_lamport() + 1;
    msg.clock_lamport = lamport_;

    if (send(client_socket_, (void *)&msg, sizeof(msg), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(client_socket_);
        exit(1);
    }
    printf("%s, %d, SEND, SHUTDOWN_NOW\n", proc_name, lamport_);
};

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

void socket_recieve(int is_server, int expected_lamport){
    DEBUG_PRINTF("Now recieving...\n");
    char *action;
    int recv_socket;
    struct message recv_msg;

    if (is_server){
        recv_socket = client_socket_;
    } else {
        recv_socket = socket_;
    }

    do{
        if (recv(recv_socket, (void *)&recv_msg, sizeof(recv_msg), 0) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            close(socket_);
            close(client_socket_);
            exit(1);
        }
    } while (expected_lamport != recv_msg.clock_lamport);

    get_action(&action);

    DEBUG_PRINTF("msg_lamport: %d | local_lamport: %d\n",
    recv_msg.clock_lamport,lamport_);

    if (recv_msg.clock_lamport > lamport_){
        lamport_ = recv_msg.clock_lamport + 1;
    }

    printf("%s, %d, RECV (%s), %s\n", proc_name, 
    recv_msg.clock_lamport, recv_msg.origin, action);
}

void socket_close(){
    close(socket_);
};

void get_action(char **action){
    switch(msg.action) {
        case 0:
            *action = "READY_TO_SHUTDOWN";
            break;

        case 1:
            *action = "SHUTDOWN_ACK";
            break;

        case 2:
            *action = "SHUTDOWN_NOW";
            break;
    }
};