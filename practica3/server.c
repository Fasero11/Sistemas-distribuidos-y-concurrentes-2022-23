// Gonzalo Vega Pérez - 2022

#include "proxy.h"

void read_output(){
    int counter_;
    char buffer[20];
    FILE *fpt;
    fpt = fopen("server_output.txt",  "r");
    if (fpt != NULL){
        while (fgets(buffer, sizeof(buffer), fpt) != NULL) {}
        fclose(fpt);
        counter_ = atoi(buffer);
    } else {
        fpt = fopen("server_output.txt",  "a");
        fclose(fpt);
        counter_ = 0;
    }
    set_counter(counter_);
}

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
    
    init_server(ip, port, priority, ratio);
    read_output();

    while (1){
        pthread_t new_thread;
        int *thread_info = malloc(50);
    
        init_server_thread(thread_info);

        if (pthread_create(&new_thread, NULL, talk_2_client, (void *)thread_info) < 0){
            warnx("Error while creating Thread\n");
            exit(EXIT_FAILURE);
        }
        
        set_current_threads(1);
    }
};