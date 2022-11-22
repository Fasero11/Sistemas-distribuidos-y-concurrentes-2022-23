// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void print_usage() {
    printf("Usage: client --ip IP --port PORT --mode writer/reader --threads N\n");
}

int main(int argc, char **argv) {
    int current_threads = 0;
    int opt = 0;
    int port;
    char *priority;
    char *ip = "0.0.0.0";
    int client_sockets[MAX_THREADS];

    static struct option long_options[] = {
        {"port",    required_argument,  NULL,  'p' },
        {"priority",    required_argument,  NULL,  'm' },
        {0,         0,                  0,     0   }
    };

    int long_index =0;
    while ((opt = getopt_long_only(argc, argv, "", long_options, &long_index )) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'm':
                priority = optarg;
                break;
            default: 
                print_usage(); 
                exit(EXIT_FAILURE);
        }
    }

    if ((strcmp(priority, "writer") != 0 && strcmp(priority, "reader") != 0)) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    //DEBUG_PRINTF("PORT: %d | PRIORITY: %s\n", port, priority);

    //DEBUG_PRINTF("Server create socket\n");
    int socket = socket_create();

    //DEBUG_PRINTF("Server set IP and Port\n");
    set_ip_port(ip,port);

    //DEBUG_PRINTF("Server binding\n");
    socket_bind(socket);

    //DEBUG_PRINTF("Server listening\n");
    socket_listen(socket);

    int i = 0;
    while (current_threads < MAX_THREADS){
        pthread_t new_thread;

        //DEBUG_PRINTF("Server accepting. Current Threads: %d\n",current_threads);
        client_sockets[i] = socket_accept(socket);

        DEBUG_PRINTF("New Thread. Client Socket: %d\n",client_sockets[i]);       
        if (pthread_create(&new_thread, NULL, talk_2_client, (void *) &client_sockets[i]) < 0){
            warnx("Error while creating Thread\n");
            exit(EXIT_FAILURE);
        }

        i++;
        current_threads++;

        //sleep(1);
    }
};