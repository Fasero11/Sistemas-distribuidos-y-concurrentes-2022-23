// Gonzalo Vega Pérez - 2022

#include "proxy_publisher.h"

void print_usage() {
    printf("Usage: publisher --ip [BROKER_IP] --port [BROKER_PORT] --topic [TOPIC]\n");
}

int main(int argc, char **argv) {
    int opt = 0;
    int port;
    char *ip;
    char *topic;

    static struct option long_options[] = {
        {"ip",      required_argument,  NULL,  'i' },
        {"port",    required_argument,  NULL,  'p' },
        {"topic",    required_argument,  NULL,  'm' },
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
                topic = optarg;
                break;
            default: 
                print_usage(); 
                exit(EXIT_FAILURE);
        }
    }

    set_name("Publisher");

    set_ip_port(ip,port);

    talk_to_server(topic);

    exit(EXIT_SUCCESS);
}
