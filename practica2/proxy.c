// Gonzalo Vega Pérez - 2022

#include "proxy.h"

struct message msg;
struct sockaddr_in addr, client_addr_1, client_addr_2;
char proc_name[2];
int socket_, client_socket_1_, client_socket_2_;
int lamport_ = 0;

int expected_lamport_p1[2] = {3,-1};
int expected_lamport_p2[4] = {1,5,9,-1};
int expected_lamport_p3[2] = {7,-1};


void signal_handler(int signal){
    DEBUG_PRINTF("CTRL + C. Closing sockets: %d, and %d... bye\n",
    client_socket_1_, client_socket_2_);
    close(client_socket_1_);
    close(client_socket_2_);
    exit(1);
}

void close_client(){
    DEBUG_PRINTF("SEND SHUTDOWN_ACK");
    notify_shutdown_ack();

    DEBUG_PRINTF("CLOSING SOCKET %d\n", socket_);
    close(socket_);
}

void close_server(){
    close(socket_);
    close(client_socket_1_);
    close(client_socket_2_);
}

void *client_receive(void *ptr){
    notify_ready_shutdown();
    // Clients receive from socket_
    int *expected_lamport;
    struct message message;
    int current_id = 0;
    char *action;

    if(strcmp(proc_name, "P1") == 0){
        expected_lamport = expected_lamport_p1;
    } else {
        expected_lamport = expected_lamport_p3;
    }

    while(1){
        DEBUG_PRINTF("WAITING LAMPORT %d\n", expected_lamport[current_id]);
        if (recv(socket_, (void *)&message, sizeof(message), 0) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            close(socket_);
            exit(1);
        }
        DEBUG_PRINTF("RECEIVED MSG %d\n", message.clock_lamport);
        // If message contains the lamport value we expect:
        if (message.clock_lamport == expected_lamport[current_id]){
            current_id++;
            if (message.clock_lamport > lamport_){
                DEBUG_PRINTF("LAMPORT SET TO  %d\n", message.clock_lamport + 1);
                lamport_ = message.clock_lamport + 1;
            }

            action_to_str(&action, message.action);
            printf("%s, %d, RECV (%s) %s\n",
            proc_name, lamport_, message.origin, action);

            // -1 Indicates end of lamport values. We don't expect more.
            if (expected_lamport[current_id] == -1){
                break;
            }
        }
    }

    pthread_exit(NULL);
}

void send_msg_to_client(int lamport){
    // SERVIDOR: P2
    // Recibe de P1 y P3 READY_TO_SHUTDOWN
    // Envía SHUTDOWN_NOW
    // Recibe SHUTDOWN_ACK de P1
    // Envía SHUTDOWN_NOW
    // Recibe SHUTDOWN_ACK de P3
    switch(lamport){
        case 1:
            notify_shutdown_now();
            break;
        case 5:
            notify_shutdown_now();
            break;
    }

}

void *server_receive(void *ptr){
    // Server receives from client_socket_1_ and client_socket_2_
    int *expected_lamport = expected_lamport_p2;
    struct message message;
    int current_id = 0;
    char *action;
    while(1){
        DEBUG_PRINTF("Waiting for lamport: %d\n", expected_lamport[current_id]);
        // Receive from both clients.
        // Until message contains the lamport value we expect:
        do{
            //DEBUG_PRINTF("recv 1\n");
            if (do_receive_in_socket(client_socket_1_, &message)){
                if (expected_lamport[current_id] == message.clock_lamport){
                    continue;
                }
            };
            //DEBUG_PRINTF("recv 2\n");
            do_receive_in_socket(client_socket_2_, &message);
        } while (message.clock_lamport != expected_lamport[current_id]);

        DEBUG_PRINTF("RECEIVED MSG %d\n", message.clock_lamport);

        if (message.clock_lamport > lamport_){
            DEBUG_PRINTF("LAMPORT SET TO  %d\n", message.clock_lamport + 1);
            lamport_ = message.clock_lamport + 1;
        }
        action_to_str(&action, message.action);
        printf("%s, %d, RECV (%s) %s\n",
        proc_name, lamport_, message.origin, action);

        DEBUG_PRINTF("SEND MSG TO CLIENT\n");
        send_msg_to_client(message.clock_lamport);

        current_id++;
        // -1 Indicates end of lamport values. We don't expect more.
        if (expected_lamport[current_id] == -1){
            break;
        }

        message.clock_lamport = 0;
    }
    pthread_exit(NULL);
}


void init_client(){
    socket_create();
    socket_connect();
}

void init_server(){
    socket_create();
    socket_bind();
    socket_listen();
    socket_accept();
}

void set_name (char name[2]){
    strcpy(proc_name,name);
};
    
void set_ip_port (char* ip, unsigned int port){
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    DEBUG_PRINTF("%s: IP set: %s | Port set: %d \n",proc_name ,ip, port);
};

int get_clock_lamport(){
    return lamport_;
};

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

void action_to_str(char **action, int in_action){
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

int do_receive_in_socket(int socket, struct message *recv_msg){
    fd_set readmask;
    struct timeval timeout;
    int is_data = 0;

    FD_ZERO(&readmask); // Reset la mascara
    FD_SET(socket, &readmask); // Asignamos el nuevo descriptor
    FD_SET(STDIN_FILENO, &readmask); // Entrada
    timeout.tv_sec=0; timeout.tv_usec=10000; // Timeout de 0.01 seg.

    if (select(socket+1, &readmask, NULL, NULL, &timeout)==-1){
        warnx("select() failed. %s\n", strerror(errno));
        close(socket);
        close(socket_);
        exit(1);
    }

    if (FD_ISSET(socket, &readmask)){
        // Datos recibidos en el socket
        if (recv(socket, (void *)recv_msg, sizeof(*recv_msg), MSG_DONTWAIT) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            close(socket);
            close(socket_);
            exit(1);
        }
        is_data = 1;
    }
    return is_data;
};