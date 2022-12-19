// Gonzalo Vega Pérez - 2022

#include "proxy_broker.h"
struct sockaddr_in addr, client_addr_;
char proc_name[32];

int server_socket, mode, current_subscriber_id, next_publisher, next_subscriber = 1,
current_publishers, current_subscribers;

struct subscriber all_subscribers[MAX_SUBSCRIBERS];
struct publisher all_publishers[MAX_PUBLISHERS];
struct topic all_topics[MAX_TOPICS];

void init_broker(char* ip, int port, char* mode_){

    //DEBUG_PRINTF("Server create socket\n");
    server_socket = socket_create();

    //DEBUG_PRINTF("Server set IP and Port\n");
    set_ip_port(ip,port);

    //DEBUG_PRINTF("Server binding\n");
    socket_bind(server_socket);

    //DEBUG_PRINTF("Server listening\n");
    socket_listen(server_socket);

    if (strcmp(mode_, "secuencial") != 0 ){
        mode = SECUENCIAL;
    } else if (strcmp(mode_, "paralelo") != 0 ){
        mode = PARALELO;
    } else if (strcmp(mode_, "justo") != 0 ){
        mode = JUSTO;
    };

    int i;
    for(i = 0; i < MAX_TOPICS; i++){
        struct topic new_topic;
        char buffer[100];
        snprintf(buffer, 100, "TOPIC %d", i);
        strcpy(new_topic.name, buffer);
        new_topic.num_publishers = 0;
        new_topic.num_subscribers = 0;
        all_topics[i] = new_topic;

        DEBUG_PRINTF("NEW TOPIC CREATED: %s\n", new_topic.name);
    }
}

void init_server_thread(int *thread_info){
    int client_socket_ = socket_accept(server_socket);
    thread_info[0] = client_socket_;
}

void set_name (char name[6]){
    strcpy(proc_name,name);
};

void set_ip_port (char* ip, unsigned int port){
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    DEBUG_PRINTF("%s: IP set: %s | Port set: %d \n",proc_name ,ip, port);
};

int socket_create(){
    setbuf(stdout, NULL);
    int socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0){
        warnx("%s: Error creating socket. %s\n",proc_name ,strerror(errno));
        exit(1); 
    }
    DEBUG_PRINTF("%s: Socket successfully created...\n",proc_name );

    return socket_;
};

void socket_connect(int socket_){
    if (connect(socket_, (struct sockaddr *)&addr, sizeof(addr)) != 0){
        DEBUG_PRINTF("Error connecting\n");
        exit(EXIT_FAILURE);
    }
    //DEBUG_PRINTF ("%s: Connected to server...\n",proc_name );
};

void socket_bind(int socket_){
    if (bind(socket_, (struct sockaddr*)&addr, 
    sizeof(addr)) < 0){
        warnx("%s: bind() failed. %s\n",proc_name , strerror(errno));
        close(socket_);
        exit(1); 
    }
    DEBUG_PRINTF("%s: Socket successfully binded...\n",proc_name );
    DEBUG_PRINTF("%s: Binded to port: %d\n",proc_name , addr.sin_port);
};

void socket_listen(int socket_){
    DEBUG_PRINTF("%s: Socket listening...\n",proc_name );
    if (listen(socket_, 1000) < 0){
        warnx("%s: listen() failed. %s\n",proc_name , strerror(errno));
        close(socket_);
        exit(1);        
    }
};

int socket_accept(int socket_){
    int client_socket_;
    socklen_t addr_size_ = sizeof(client_addr_);
    client_socket_ = accept(socket_, (struct sockaddr*)&client_addr_, 
    &addr_size_);
    DEBUG_PRINTF("%s: Client Connected!. client_socket_: %d\n",proc_name ,client_socket_);

    if (client_socket_ < 0){
        warnx("%s: accept() failed. %s\n",proc_name , strerror(errno));
        close(client_socket_);
        close(socket_);
        exit(1);
    }

    return client_socket_;
};

void *talk_to_client(void *ptr){

    struct response response;

    int *thread_info = (int*)ptr;
    int client_socket_ = thread_info[0];

    struct timespec time;

    // Variables for printf //
    long time_seconds;
    long time_nanoseconds;
    int id;
    char *client_type;
    char *topic;
    //////////////////////////

    // Accept connection
    // Wait for register message
    // Once received, check if its publisher or subscriber and save info.
    // Print message 

    DEBUG_PRINTF("Waiting for register...\n");
    struct message message;
    if (recv(client_socket_, (void *)&message, sizeof(message), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        exit(1);
    }

    DEBUG_PRINTF("New message: action: %d, topic: %s\n",message.action, message.topic);

    clock_gettime(CLOCK_MONOTONIC, &time);

    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;

    if (message.action == REGISTER_SUBSCRIBER){
        struct subscriber new_subscriber;
        new_subscriber.fd = client_socket_;
        strcpy(new_subscriber.topic, message.topic);
        new_subscriber.id = current_subscriber_id++;
        all_subscribers[next_subscriber] = new_subscriber;

        id = new_subscriber.fd;
        client_type = "Suscriptor";
        topic = new_subscriber.topic;

        if (next_subscriber < MAX_SUBSCRIBERS){
            response.id = next_subscriber++;
            response.response_status = OK;
        } else {
            response.id = -1;
            response.response_status = LIMIT;         
        }

        DEBUG_PRINTF("NEW SUBSCRIBER: fd: %d, topic: %s, id: %d\n",
        new_subscriber.fd, new_subscriber.topic, new_subscriber.id);

    } else if (message.action == REGISTER_PUBLISHER){
        struct publisher new_publisher;
        new_publisher.fd = client_socket_;
        strcpy(new_publisher.topic, message.topic);
        all_publishers[next_publisher] = new_publisher;

        id = next_publisher;
        client_type = "Publicador";
        topic = new_publisher.topic;

        if (next_publisher < MAX_PUBLISHERS){
            response.id = next_publisher++;
            response.response_status = OK;
        } else {
            response.id = -1;
            response.response_status = LIMIT;         
        }

        DEBUG_PRINTF("NEW PUBLISHER: fd: %d, topic: %s\n",
        new_publisher.fd, new_publisher.topic);
    }


    if (send(client_socket_, (void *)&response, sizeof(response), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    printf("[%ld.%ld] Nuevo Cliente (%d) %s conectado : %s\n Resumen:\n",
    time_seconds, time_nanoseconds, id, client_type, topic);

    int i;
    for(i = 0; i < MAX_TOPICS; i++){

        //DEBUG_PRINTF("%s | %s\n", topic, all_topics[i].name);

        if (strcmp(topic, all_topics[i].name) == 0){
            if (strcmp(client_type, "Publicador") == 0){
                all_topics[i].num_publishers++;
            }  else {
                all_topics[i].num_subscribers++;
            }
        }

        printf("\t%s: %d Suscriptores - %d Publicadores\n",
        all_topics[i].name, all_topics[i].num_subscribers, all_topics[i].num_publishers);
    }

    pthread_exit(NULL);  
};