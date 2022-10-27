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
#define IP "127.0.0.1"
#define BUFFER_SIZE 1024

int sock;

void sighandler(int signum) {
    DEBUG_PRINTF("Closing Client\n");
    close(sock);
    DEBUG_PRINTF("Exiting\n");
    exit(1);
}

void send_message(char *out_buffer){
    /// Send Message ///
    printf("> ");
    if (fgets(out_buffer, BUFFER_SIZE, stdin) == NULL){
        warnx("fgets() failed. %s\n", strerror(errno));
        close(sock);
        exit(1);
    }

    if (send(sock, out_buffer, sizeof(out_buffer), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        close(sock);
        exit(1);
    }
}

void receive_message(char *in_buffer, fd_set readmask, struct timeval timeout){
    FD_ZERO(&readmask); // Reset la mascara
    FD_SET(sock, &readmask); // Asignamos el nuevo descriptor
    FD_SET(STDIN_FILENO, &readmask); // Entrada
    timeout.tv_sec=0; timeout.tv_usec=500000; // Timeout de 0.5 seg.

    // Wait for read data
    DEBUG_PRINTF("Waiting for %ld\n",timeout.tv_usec);
    if (select(sock+1, &readmask, NULL, NULL, &timeout) < 0){
        warnx("select() failed. %s\n", strerror(errno));
        close(sock);
        exit(1);
    }
    DEBUG_PRINTF("Done waiting for %ld\n",timeout.tv_usec);

    if (FD_ISSET(sock, &readmask)){
        /// Receive Message ///
        DEBUG_PRINTF("Receive\n");
        in_buffer[0] = '\0';
        if (recv(sock, in_buffer, sizeof(in_buffer), MSG_DONTWAIT) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            close(sock);
            exit(1);
        }
        printf("+++ %s", in_buffer);
    }
}

int main(int argc, char **args)
{
    setbuf(stdout, NULL);

    struct sockaddr_in addr;
    char in_buffer[BUFFER_SIZE];
    char out_buffer[BUFFER_SIZE];

    void sighandler(int);
    signal(SIGINT, sighandler);

    fd_set readmask;
    struct timeval timeout;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0){
        warnx("Error creating socket. %s\n", strerror(errno));
        exit(1);
    }

    printf("Socket successfully created...\n");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);

    while (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0){
        DEBUG_PRINTF("Server not found\n");
    }

    printf("connected to the server...\n");

    DEBUG_PRINTF("Send 1\n");

    while (1){   
        send_message(out_buffer);

        receive_message(in_buffer, readmask, timeout);
    }
}