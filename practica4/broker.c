// Gonzalo Vega Pérez - 2022

#include "proxy_broker.h"

void print_usage() {
    printf("Usage: broker --port [BROKER_PORT] --mode [secuencial/paralelo/justo] \n");
}

int main(int argc, char **argv) {
    int opt = 0;
    int port;
    char *mode;
    char *ip = "0.0.0.0";

    static struct option long_options[] = {
        {"port",    required_argument,  NULL,  'p' },
        {"mode",    required_argument,  NULL,  'm' },
        {0,         0,                  0,     0   }
    };

    int long_index = 0;
    while ((opt = getopt_long_only(argc, argv, "", long_options, &long_index )) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'm': 
                mode = optarg;
                break;
            default: 
                print_usage(); 
                exit(EXIT_FAILURE);
        }
    }

    if ((strcmp(mode, "secuencial") != 0 && strcmp(mode, "paralelo") != 0 && strcmp(mode, "justo") != 0)) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    
    //.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.

    init_server(ip, port, mode);

    pthread_t new_thread;

    if (pthread_create(&new_thread, NULL, talk_to_client, (void *)NULL) < 0){
        warnx("Error while creating Thread\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(new_thread, NULL) < 0) {
        warnx("Error while joining Thread\n");
        exit(1);
    };

};