// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void print_usage() {
    printf("Usage: client --ip IP --port PORT --mode writer/reader --threads N\n");
}

int main(int argc, char **argv) {
    int opt = 0;
    int port;
    char *priority;
    char *ip = "0.0.0.0";

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

    DEBUG_PRINTF("PORT: %d | PRIORITY: %s\n", port, priority);

    DEBUG_PRINTF("Server create socket\n");
    socket_create();

    DEBUG_PRINTF("Server set IP and Port\n");
    set_ip_port(ip,port);

    DEBUG_PRINTF("Server binding\n");
    socket_bind();

    DEBUG_PRINTF("Server listening\n");
    socket_listen();

    DEBUG_PRINTF("Server accepting\n");
    socket_accept();

    DEBUG_PRINTF("Server receiving\n");
    struct request request = receive_request();

    DEBUG_PRINTF("Server doing request\n");
    struct response response = do_request(request);

    DEBUG_PRINTF("Server Sending respose\n");
    send_response(response);

    close_server();

    exit(EXIT_SUCCESS);
};