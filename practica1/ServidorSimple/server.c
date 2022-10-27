#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <err.h>


#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#define PORT 8086
#define BUFFER_SIZE 1024

int server_socket;

void sighandler(int signum) {
    DEBUG_PRINTF("Closing Server\n");
    DEBUG_PRINTF("socket_id (close): %d\n", server_socket);
    close(server_socket);
    DEBUG_PRINTF("Exiting\n");
    exit(1);
}

void send_message(char *out_buffer, int client_socket){
    /// Send Message ///
    DEBUG_PRINTF("Send\n");
    printf("> ");
    if (fgets(out_buffer, BUFFER_SIZE, stdin) == NULL){
        warnx("fgets() failed. %s\n", strerror(errno));
        close(server_socket);
        close(client_socket);
        exit(1);
    }
    if (send(client_socket, out_buffer, sizeof(out_buffer), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(server_socket);
        close(client_socket);
        exit(1);
    }
}

void receive_message(char * in_buffer, int client_socket){
    /// Receive Message ///
    DEBUG_PRINTF("Receive\n");
    in_buffer[0] = '\0';
    DEBUG_PRINTF("%s\n",in_buffer);
    DEBUG_PRINTF("BufferSize: %ld. client_socket %d\n", sizeof(in_buffer),client_socket);
    if (recv(client_socket, in_buffer, sizeof(in_buffer), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        close(server_socket);
        close(client_socket);
        exit(1);
    }
    printf("+++ %s", in_buffer);
}

int main(int argc, char **args) {
    setbuf(stdout, NULL);

    char out_buffer[BUFFER_SIZE];
    char in_buffer[BUFFER_SIZE];
    int client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    
    signal(SIGINT, sighandler);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    DEBUG_PRINTF("socket_id: %d\n", server_socket);
    if (server_socket < 0){
        warnx("Error creating socket. %s\n",strerror(errno));
        exit(1); 
    }

    printf("Socket successfully created...\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr*)&server_addr, 
    sizeof(server_addr)) < 0){
        warnx("bind() failed. %s\n", strerror(errno));
        close(server_socket);
        exit(1); 
    }
        
    printf("Socket successfully binded...\n");
    DEBUG_PRINTF("Binded to port: %d\n", PORT);

    printf("Socket listening...\n");
    if (listen(server_socket, 5) < 0){
        warnx("listen() failed. %s\n", strerror(errno));
        close(server_socket);
        exit(1);        
    }

    addr_size = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, 
    &addr_size);
    if (client_socket < 0){
        warnx("accept() failed. %s\n", strerror(errno));
        close(client_socket);
        close(server_socket);
        exit(1);
    }
    DEBUG_PRINTF("Client Connected!. client_socket: %d\n",client_socket);

    while(1){

        receive_message(in_buffer, client_socket);

        send_message(out_buffer, client_socket);
    }
}