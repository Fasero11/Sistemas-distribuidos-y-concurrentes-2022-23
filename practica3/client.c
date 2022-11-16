// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void print_usage() {
    printf("Usage: client --ip IP --port PORT --mode writer/reader --threads N\n");
}

int main(int argc, char **argv) {
    int opt = 0;
    int port, threads;
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
                DEBUG_PRINTF("ip: %s\n",ip);
                break;
            case 'p':
                port = atoi(optarg);
                DEBUG_PRINTF("port: %d\n",port);
                break;
            case 'm':
                mode = optarg;
                DEBUG_PRINTF("mode: %s\n",mode);
                break;
            case 't':
                threads = atoi(optarg);
                DEBUG_PRINTF("threads: %d\n", threads);
                break;
            default: print_usage(); 
                 exit(EXIT_FAILURE);
        }
    }

    // if (threads <= 0 || strcmp(mode, "writer") != 0 || strcmp(mode, "reader") != 0  ) {
    //     DEBUG_PRINTF("Argument Failure\n");
    //     print_usage();
    //     exit(EXIT_FAILURE);
    // }

    DEBUG_PRINTF("IP: %s | PORT: %d | MODE: %s | THREADS: %d\n", ip, port, mode, threads);

    exit(EXIT_SUCCESS);
}
