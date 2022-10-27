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
#include <pthread.h>

#ifdef DEBUG
    #define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#define BUFFER_SIZE 1024
#define NUMBER_OF_THREADS 100

int server_socket;

void sighandler(int signum) {
    DEBUG_PRINTF("Closing Server\n");
    DEBUG_PRINTF("socket_id (close): %d\n", server_socket);
    close(server_socket);
    DEBUG_PRINTF("Exiting\n");
    exit(1);
}

void* talk_to_client(void* ptr){

    int client_socket =  *((int *)ptr);
    char in_buffer[BUFFER_SIZE];
    /// Receive Message ///
    //DEBUG_PRINTF("Receive. client_Socket: %d\n",client_socket);
    if (recv(client_socket, in_buffer, BUFFER_SIZE, 0) < 0){
        warnx("recv() from fd %d failed. %s\n",client_socket, strerror(errno));
        exit(1);  
    }
    printf("+++ %s\n", in_buffer);

    /// Send Message ///
    char *out_buffer = "Hello client!";
    DEBUG_PRINTF("Sending. %s\n",out_buffer);
    if (send(client_socket, out_buffer, strlen(out_buffer)+1, 0) < 0){
        warnx("send() failed. %s\n",strerror(errno));
        exit(1);
    }

    pthread_exit(NULL);
}

int main(int argc, char **args) {
    setbuf(stdout, NULL);
    
    /// Extract Parameters ///
    int port;
    if (argc == 2){
        port = atoi(args[1]);
    } else {
        warnx("usage: server port");
        exit(1);
    }
    if ((port < 1024) || (port > 65535)){
        warnx("invalid port. Port must be a number between 1024 and 65535");
        exit(1);
    }
    //////////////////////////

    pthread_t threads[NUMBER_OF_THREADS];
    int fds[NUMBER_OF_THREADS];

    int current_thread;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    signal(SIGINT, sighandler);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        warnx("Error creating socket. %s\n",strerror(errno));
        exit(1);
    }

    printf("Socket successfully created...\n");

    /// Configure Server Socket ///
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ///////////////////////////////

    if (bind(server_socket, (struct sockaddr*)&server_addr, 
    sizeof(server_addr)) < 0){
        warnx("bind() failed. %s\n",strerror(errno));
        close(server_socket);
        exit(1);
    }
        
    printf("Socket successfully binded...\n");
    DEBUG_PRINTF("Binded to port: %d\n", port);

    if (listen(server_socket, 5) < 0){
        warnx("listen() failed. %s\n",strerror(errno));
        close(server_socket);
        exit(1);
    }

    printf("Socket listening...\n");

    addr_size = sizeof(client_addr);
    current_thread = 0;

    while(1){
        DEBUG_PRINTF("current_thread: %d\n", current_thread);
        // If there are 100 file descriptor opened (100 threads)
        if (current_thread >= NUMBER_OF_THREADS){
            // join all threads, then close all file descriptors
            DEBUG_PRINTF("RESETING\n");
            int i;
            for (i = 0; i < NUMBER_OF_THREADS; i++){
                if (pthread_join(threads[i], NULL) < 0) {
                    warnx("Error while joining Thread\n");
                    close(server_socket);
                    exit(1);
                }
            }
            for (i = 0; i < NUMBER_OF_THREADS; i++){
                if (close(fds[i]) < 0){
                    warnx("close() failed. %s\n",strerror(errno));
                    close(server_socket);
                    exit(1);
                }
            }
            current_thread = 0;
        }

        // Accept connection
        fds[current_thread] = accept(server_socket, 
        (struct sockaddr*)&client_addr, &addr_size);
        if (fds[current_thread] < 0){
            warnx("accept() failed. %s\n",strerror(errno));
            close(server_socket);
            exit(1);
        }

        // Create Thread
        if (pthread_create(&threads[current_thread], NULL, talk_to_client, 
        &fds[current_thread]) < 0){
            warnx("Error while creating Thread\n");
            close(server_socket);
            exit(1);
        }
        current_thread += 1;
    }
}