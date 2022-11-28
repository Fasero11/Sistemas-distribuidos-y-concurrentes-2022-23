// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void print_usage() {
    printf("Usage: client --ip IP --port PORT --mode writer/reader --threads N --ratio M\n");
}

int main(int argc, char **argv) {
    int opt = 0;
    int port;
    char *priority;
    int ratio = 0;
    char *ip = "0.0.0.0";

    static struct option long_options[] = {
        {"port",    required_argument,  NULL,  'p' },
        {"priority",    required_argument,  NULL,  'm' },
        {"ratio",    required_argument,  NULL,  'r' },
        {0,         0,                  0,     0   }
    };

    int long_index = 0;
    while ((opt = getopt_long_only(argc, argv, "", long_options, &long_index )) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'm':
                priority = optarg;
                break;
            case 'r':
                ratio = atoi(optarg);
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
    DEBUG_PRINTF("C\n");
    //DEBUG_PRINTF("PORT: %d | PRIORITY: %s\n", port, priority);

    //DEBUG_PRINTF("Server create socket\n");
    int socket = socket_create();

    //DEBUG_PRINTF("Server set IP and Port\n");
    set_ip_port(ip,port);

    //DEBUG_PRINTF("Server binding\n");
    socket_bind(socket);

    //DEBUG_PRINTF("Server listening\n");
    socket_listen(socket);

    set_priority(priority);

    set_ratio(ratio);

    read_output();

    while (1){
        pthread_t new_thread;

        int client_socket_ = socket_accept(socket);
        int id = get_free_fd();
        set_value_fd (id, client_socket_);

        int *values = malloc(50);
        values[0] = id;
        values[1] = client_socket_;

        if (pthread_create(&new_thread, NULL, talk_2_client, (void *)values) < 0){
            warnx("Error while creating Thread\n");
            exit(EXIT_FAILURE);
        }
        
        set_current_threads(1);
    }
};