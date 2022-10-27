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

#define BUFFER_SIZE 1024

int sock;

void sighandler(int signum) {
    DEBUG_PRINTF("Closing Client\n");
    DEBUG_PRINTF("socket_id (close): %d\n", sock);
    close(sock);
    DEBUG_PRINTF("Exiting\n");
    exit(1);
}

void send_message(char *out_buffer){
    /// Send Message ///
    if (send(sock, out_buffer, strlen(out_buffer)+1, 0) < 0){
        warnx("send() failed. %s\n",strerror(errno));
        close(sock);
        exit(1);
    }
    free(out_buffer);
}

void receive_message(char *in_buffer){
    /// Receive Message ///
    DEBUG_PRINTF("Receive\n");
    in_buffer[0] = '\0';
    if (recv(sock, in_buffer, BUFFER_SIZE, 0) < 0){
        warnx("recv() failed. %s\n",strerror(errno));
        close(sock);
        exit(1);  
    }
    DEBUG_PRINTF("Received: %s\n",in_buffer);
    printf("+++ %s\n", in_buffer);
}

int main(int argc, char **args){
    setbuf(stdout, NULL);

    /// Extract Parameters ///
    char *ip;
    int port;
    if (argc == 4){
        ip = args[2];
        port = atoi(args[3]);
    } else {
         warnx("usage: client client_id ip port");
         exit(1);
    }
    if ((port < 1024) || (port > 65535)){
        warnx("invalid port. Port must be a number between 1024 and 65535");
        exit(1);
    }
    int client_id = atoi(args[1]);
    //////////////////////////

    struct sockaddr_in addr;
    char in_buffer[BUFFER_SIZE];

    signal(SIGINT, sighandler);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0){
        warnx("Error creating socket\n");
        exit(1);
    }

    printf("Socket successfully created...\n");

    /// Configure client socket ///
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    ///////////////////////////////

    while (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0){
        DEBUG_PRINTF("Server not found\n");
    }

    printf("connected to the server...\n");

    while (1)
    {   
        char *out_buffer = malloc(snprintf(NULL, 0, "Hello server! From client: %d", client_id));
        if (out_buffer == NULL) {
            warnx("malloc failed. %s\n",strerror(errno));
            close(sock);
            exit(1);
        }

        if (sprintf(out_buffer, "Hello server! From client: %d", client_id) < 0) {
            warnx("sprintf() failed. %s\n",strerror(errno));
            close(sock);
            exit(1);
        }

        DEBUG_PRINTF("out_buffer: %s | size: %ld\n", out_buffer, strlen(out_buffer));

        send_message(out_buffer);
        
        receive_message(in_buffer);
        return 0;
    }
}