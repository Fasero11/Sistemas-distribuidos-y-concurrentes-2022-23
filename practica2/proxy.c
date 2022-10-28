// Gonzalo Vega Pérez - 2022

#include "proxy.h"

struct message msg;
struct sockaddr_in addr, client_addr_1, client_addr_2;
char proc_name[2];
int socket_, client_socket_1_, client_socket_2_;
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

    if (send(client_socket_1_, (void *)&msg, sizeof(msg), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(client_socket_1_);
        exit(1);
    }
    
    if (send(client_socket_2_, (void *)&msg, sizeof(msg), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(client_socket_2_);
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
    socklen_t addr_size_1 = sizeof(client_addr_1);
    client_socket_1_ = accept(socket_, (struct sockaddr*)&client_addr_1, 
    &addr_size_1);
    DEBUG_PRINTF("%s: Client Connected!. client_socket_: %d\n",proc_name ,client_socket_1_);

    socklen_t addr_size_2 = sizeof(client_addr_1);
    client_socket_2_ = accept(socket_, (struct sockaddr*)&client_addr_2, 
    &addr_size_2);
    DEBUG_PRINTF("%s: Client Connected!. client_socket_: %d\n",proc_name ,client_socket_2_);

    if ((client_socket_1_ < 0) || (client_socket_2_ < 0)){
        warnx("%s: accept() failed. %s\n",proc_name , strerror(errno));
        close(client_socket_1_);
        close(client_socket_2_);
        close(socket_);
        exit(1);
    }

};

void socket_receive(int is_server, int expected_lamport){
    DEBUG_PRINTF("Now receiving...\n");
    char *action;
    struct message recv_msg;
    
    if (is_server){
        do{
            do_receive_in_socket(client_socket_1_, &recv_msg);
            do_receive_in_socket(client_socket_2_, &recv_msg);
    } while (expected_lamport != recv_msg.clock_lamport);
    } else {
        do{
            if (recv(socket_, (void *)&recv_msg, sizeof(recv_msg), 0) < 0){
                warnx("recv() failed. %s\n", strerror(errno));
                close(socket_);
                exit(1);
            }
        } while (expected_lamport != recv_msg.clock_lamport);
    };

    get_action(&action, recv_msg.action);

    DEBUG_PRINTF("msg_lamport: %d | local_lamport: %d\n",
    recv_msg.clock_lamport,lamport_);

    if (recv_msg.clock_lamport > lamport_){
        lamport_ = recv_msg.clock_lamport + 1;
    }

    printf("%s, %d, RECV (%s), %s\n", proc_name, 
    lamport_, recv_msg.origin, action);
}

void socket_close(){
    close(socket_);
};

void get_action(char **action, int in_action){
    DEBUG_PRINTF("Action: %d\n",in_action);
    switch(in_action) {
        case 0:
            *action = "READY_TO_SHUTDOWN";
            break;

        case 1:
            *action = "SHUTDOWN_NOW";
            break;

        case 2:
            *action = "SHUTDOWN_ACK";
            break;
    }
};

void do_receive_in_socket(int socket, struct message *recv_msg){
    fd_set readmask;
    struct timeval timeout;

    FD_ZERO(&readmask); // Reset la mascara
    FD_SET(socket, &readmask); // Asignamos el nuevo descriptor
    FD_SET(STDIN_FILENO, &readmask); // Entrada
    timeout.tv_sec=0; timeout.tv_usec=500000; // Timeout de 0.5 seg.

    if (select(socket+1, &readmask, NULL, NULL, &timeout)==-1){
        warnx("select() failed. %s\n", strerror(errno));
        close(socket);
        close(socket_);
        exit(1);
    }

    if (FD_ISSET(socket, &readmask)){
        // Data received in socket
        if (recv(socket, (void *)recv_msg, sizeof(*recv_msg), MSG_DONTWAIT) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            close(socket);
            close(socket_);
            exit(1);
        }
    }
};