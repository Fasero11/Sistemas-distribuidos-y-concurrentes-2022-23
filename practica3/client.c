// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void print_usage() {
    printf("Usage: client --ip IP --port PORT --mode writer/reader --threads N\n");
}

int main(int argc, char **argv) {
    int opt = 0;
    int port, thread_num;
    char *ip;
    char *mode;

    static struct option long_options[] = {
        {"ip",      required_argument,  NULL,  'i' },
        {"port",    required_argument,  NULL,  'p' },
        {"mode",    required_argument,  NULL,  'm' },
        {"threads", required_argument,  NULL,  't' },
        {0,         0,                  0,     0   }
    };

    int long_index =0;
    while ((opt = getopt_long_only(argc, argv, "", long_options, &long_index )) != -1) {
        switch (opt) {
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'm':
                mode = optarg;
                break;
            case 't':
                thread_num = atoi(optarg);
                break;
            default: 
                print_usage(); 
                exit(EXIT_FAILURE);
        }
    }

    if (thread_num <= 0 || (strcmp(mode, "writer") != 0 && strcmp(mode, "reader") != 0)) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    DEBUG_PRINTF("IP: %s | PORT: %d | MODE: %s | THREADS: %d\n", ip, port, mode, thread_num);
    
    struct client_threads client_threads[thread_num];
    pthread_t all_threads[thread_num];

    set_name("Client");

    DEBUG_PRINTF("Client create socket\n");
    socket_create();

    DEBUG_PRINTF("Client set IP and Port\n");
    set_ip_port(ip,port);

    int i;
    for (i = 0; i < thread_num; i++){
        client_threads[i].mode = mode;
        client_threads[i].thread_id = i;
        if (pthread_create(&all_threads[i], NULL, talk_2_server, (void *)&client_threads[i]) < 0){
            warnx("Error while creating Thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < thread_num; i++){
        if (pthread_join(all_threads[i], NULL) < 0) {
            warnx("Error while joining Thread\n");
            exit(1);
        };
    }

    close_client();

    exit(EXIT_SUCCESS);
}
