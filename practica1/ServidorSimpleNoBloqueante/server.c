#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <err.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#define PORT 8090
#define BUFFER_SIZE 1024

int server_socket;

void sighandler(int signum) {
    DEBUG_PRINTF("Closing Server\n");
    close(server_socket);
    DEBUG_PRINTF("Exiting\n");
    exit(1);
}

void send_message(char *out_buffer, int client_socket){
    /// Send Message ///
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

void receive_message(char *in_buffer, int client_socket, 
fd_set readmask, struct timeval timeout){

    FD_ZERO(&readmask); // Reset la mascara
    FD_SET(client_socket, &readmask); // Asignamos el nuevo descriptor
    FD_SET(STDIN_FILENO, &readmask); // Entrada
    timeout.tv_sec=0; timeout.tv_usec=500000; // Timeout de 0.5 seg.

    DEBUG_PRINTF("Waiting for %ld\n",timeout.tv_usec);
    if (select(client_socket+1, &readmask, NULL, NULL, &timeout)==-1){
        warnx("select() failed. %s\n", strerror(errno));
        close(server_socket);
        close(client_socket);
        exit(1);
    }
    DEBUG_PRINTF("Done waiting for %ld\n",timeout.tv_usec);

    if (FD_ISSET(client_socket, &readmask)){
        /// Receive Message ///
        in_buffer[0] = '\0';
        if (recv(client_socket, in_buffer, sizeof(in_buffer), 
        MSG_DONTWAIT) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            close(server_socket);
            close(client_socket);
            exit(1);
        }
        printf("+++ %s", in_buffer);
    }
}

int main(int argc, char **args) {
    setbuf(stdout, NULL);

    int client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    char in_buffer[BUFFER_SIZE];
    char out_buffer[BUFFER_SIZE];

    void sighandler(int);
    signal(SIGINT, sighandler);

    fd_set readmask;
    struct timeval timeout;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        warnx("Error creating socket. %s\n", strerror(errno));
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
        close(server_socket);
        close(client_socket);
        exit(1);
    }
    DEBUG_PRINTF("Client Connected!\n");

    while(1){
        receive_message(in_buffer, client_socket, readmask, timeout);

        send_message(out_buffer, client_socket);
    }
}