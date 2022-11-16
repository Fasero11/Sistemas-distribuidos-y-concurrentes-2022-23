// Gonzalo Vega Pérez - 2022

#include "proxy.h"

struct sockaddr_in addr, client_addr_1, client_addr_2;
char proc_name[2];
int socket_, client_socket_1_, client_socket_2_;

void set_name (char name[2]){
    strcpy(proc_name,name);
};

void test(){
    DEBUG_PRINTF("Test3\n");   
}

void set_ip_port (char* ip, unsigned int port){
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    DEBUG_PRINTF("%s: IP set: %s | Port set: %d \n",proc_name ,ip, port);
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