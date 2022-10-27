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
#define IP "127.0.0.1"
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

void receive_message(char *in_buffer){
    /// Receive Message ///
    DEBUG_PRINTF("Receive\n");
    in_buffer[0] = '\0';
    if (recv(sock, in_buffer, sizeof(in_buffer), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        close(sock);
        exit(1);
    }
    printf("+++ %s", in_buffer);
}

int main(int argc, char **args)
{
    setbuf(stdout, NULL);

    struct sockaddr_in addr;
    char in_buffer[BUFFER_SIZE];
    char out_buffer[BUFFER_SIZE];

    signal(SIGINT, sighandler);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    DEBUG_PRINTF("socket_id: %d\n", sock);
    if (socket < 0)
    {
        warnx("Error creating socket. %s\n",strerror(errno));
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

    while (1){
        send_message(out_buffer);

        receive_message(in_buffer);
    }
}