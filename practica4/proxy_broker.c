// Gonzalo Vega Pérez - 2022

#include "proxy_broker.h"
struct sockaddr_in addr, client_addr_;
char proc_name[32];

int server_socket, mode, next_publisher, next_subscriber = 1,
current_publishers, current_subscribers, current_topics;

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
}

int create_topic(char *name){

    if (current_topics < MAX_TOPICS){
        // CHECK IF TOPIC ALREADY EXISTS
        int i;
        for(i = 0; i < current_topics; i++){
            if (strcmp(all_topics[i].name, name) == 0){
                DEBUG_PRINTF("Topic %s already exists.\n", name);
                // TOPIC ALREADY EXISTS
                return 2;
            }
        }

        struct topic new_topic;
        strcpy(new_topic.name, name);
        new_topic.num_publishers = 0;
        new_topic.num_subscribers = 0;
        all_topics[current_topics++] = new_topic;

        DEBUG_PRINTF("NEW TOPIC CREATED: %s  ID: %d\n",
         new_topic.name, current_topics-1);

        return 0; // TOPIC CREATED
    } else {
        return 1; // UNABLE TO CREATE TOPIC. LIMIT REACHED
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

    struct response response;               // Response structure that will be sent to the client

    int *thread_info = (int*)ptr;           
    int client_socket_ = thread_info[0];    // FD used to communicate with the client.

    struct timespec time;                   // Time structure used in print messages

    // Variables for printing messages //
    long time_seconds;
    long time_nanoseconds;
    int id;
    char *client_type;
    char *topic;
    /////////////////////////////////////

    // Wait for the client to send a register message.
    DEBUG_PRINTF("Waiting for register...\n");
    struct message message;
    if (recv(client_socket_, (void *)&message, sizeof(message), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        exit(1);
    }

    DEBUG_PRINTF("New message: action: %d, topic: %s\n",message.action, message.topic);

    // Get time when the message was recieved for printing
    clock_gettime(CLOCK_MONOTONIC, &time);
    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;

    // If the client is a Subscriber
    if (message.action == REGISTER_SUBSCRIBER){
        // Try to create the topic the Subscriber is requesting.
        int create_topic_status = create_topic(message.topic);
        // If the topic was created or already existed.
        if (create_topic_status != 1){
            // If the subscribers limit hasn't been reached yet.
            if (current_subscribers < MAX_SUBSCRIBERS){
                // Create a new subscriber structure
                struct subscriber new_subscriber;
                new_subscriber.fd = client_socket_;                 // Fill the FD used to communicate with this subscriber
                strcpy(new_subscriber.topic, message.topic);        // Fill the topic this subscriber is listening from.
                new_subscriber.id = next_subscriber;                // Fill the ID of this subscriber.
                all_subscribers[next_subscriber] = new_subscriber;  // Add this subscriber to the list of all subscribers.

                // Fill variables for printing message.
                id = next_subscriber;
                client_type = "Suscriptor";
                topic = new_subscriber.topic;

                // Fill response message variables.
                response.id = next_subscriber++;
                response.response_status = OK;

                DEBUG_PRINTF("NEW SUBSCRIBER: fd: %d, topic: %s, id: %d\n",
                new_subscriber.fd, new_subscriber.topic, new_subscriber.id);

            } else {
                // REACHED SUBSCRIBERS LIMIT
                response.id = -1;
                response.response_status = LIMIT;  
            }
        // If the topic was NOT created.
        } else if (create_topic_status == 1){
            // REACHED TOPIC LIMIT
            response.id = -1;
            response.response_status = ERROR; 
        }

    // If the client is a Publisher
    } else if (message.action == REGISTER_PUBLISHER){

        // Check if the topic the Publisher wants to use exists.
        int i;
        int topic_exists = 0;
        for(i = 0; i < current_topics; i++){
            if (strcmp(all_topics[i].name, message.topic) == 0){
                DEBUG_PRINTF("Topic %s already exists.\n", message.topic);
                topic_exists = 1;
                break;
            }
        }

        // If the topic exists
        if(topic_exists){
            // If the publishers limit hasn't been reached yet.
            if (next_publisher < MAX_PUBLISHERS){
                // Create new publisher structure
                struct publisher new_publisher;
                new_publisher.fd = client_socket_;              // Fill the FD used to communicate with this publisher
                strcpy(new_publisher.topic, message.topic);     // Fill the topic this publisher is writing to.
                new_publisher.id = next_publisher;              // Fill the ID of this publisher.
                all_publishers[next_publisher] = new_publisher; // Add this publisher to the list of all publisher. 

                // Fill variables for printing message.
                id = next_publisher;
                client_type = "Publicador";
                topic = new_publisher.topic;

                // Fill response message variables.
                response.id = next_publisher++;
                response.response_status = OK;

                DEBUG_PRINTF("NEW PUBLISHER: fd: %d, topic: %s\n",
                new_publisher.fd, new_publisher.topic);

            } else {
                // REACHED PUBLISHERS LIMIT
                response.id = -1;
                response.response_status = LIMIT;    
            }

        // If the topic does not exist. 
        } else {
            DEBUG_PRINTF("Publisher not registered. Topic %s does not exist\n",
            message.topic);
            response.id = -1;
            response.response_status = ERROR;      
        }
    }

    // SEND RESPONSE TO CLIENT
    if (send(client_socket_, (void *)&response, sizeof(response), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    // IF THE CLIENT WAS REGISTERED. (response.id != -1)
    // Print information and summary of topics, subscribers and publishers.
    if (response.id != -1){
        printf("[%ld.%ld] Nuevo Cliente (%d) %s conectado : %s\n Resumen:\n",
        time_seconds, time_nanoseconds, id, client_type, topic);

        int i;
        for(i = 0; i < current_topics; i++){

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
    }

    pthread_exit(NULL);  
};