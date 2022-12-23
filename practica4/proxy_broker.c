// Gonzalo Vega Pérez - 2022

#include "proxy_broker.h"
struct sockaddr_in addr, client_addr_;
char proc_name[32];

int server_socket, mode, next_publisher, next_subscriber = 1,
current_publishers, current_subscribers, current_topics;

struct topic all_topics[MAX_TOPICS];

int just_threads;
sigset_t just_signal;

pthread_mutex_t mutex;
pthread_cond_t just_condition;

int get_topic_id(char *topic){
    int topic_id = 0;
    for(topic_id = 0; topic_id < current_topics; topic_id++){
        if (strcmp(all_topics[topic_id].name, topic) == 0){
            break;
        }
    }

    return topic_id;
}

void sighandler(int signum){
    DEBUG_PRINTF("SIGNAL RECEIVED...  bye.\n");
    close(server_socket);
    exit(1);
}

void shift_client_list(struct topic topic, int action, int id){
    DEBUG_PRINTF("SHIFTING LIST\n");
    int i;
    // Subscriber ID will be its position in the array + 1;
    // Publisher ID will be its position in the array.

    // Every client from the one we want to delete to the last one will be
    // replaced by the one after it.
    // If we have [0, 1, 2, 3, 4] and we want to delete 2, we do:
    // [0,1,2,3,4] -> [0,1,3,3,4] -> [0,1,3,4,4]
    // The last element will be ignored because we reduce the size of the array by 1.
    // In this case the size was "reduced" by substracting one from
    // num_subscribers / num_publishers which are the variables we use to iterate.
    if (action == UNREGISTER_SUBSCRIBER){
        for(i = id-1; i < MAX_SUBSCRIBERS-1; i++){
            topic.subscribers[i] = topic.subscribers[i+1];  
        }
    } else if (action == UNREGISTER_PUBLISHER){
        for(i = id; i < MAX_PUBLISHERS-1; i++){
            topic.publishers[i] = topic.publishers[i+1];  
        }
    }

}

void unregister(int action, char *topic_name, int id){
    DEBUG_PRINTF("UNREGISTER %d: Topic: %s, ID: %d\n", action, topic_name, id);
    int topic_id = get_topic_id(topic_name);

    struct topic topic = all_topics[topic_id];

    DEBUG_PRINTF("UNREGISTER TOPIC: %s. Topic ID: %d. Subs: %d, Pubs: %d\n",
     topic.name, topic_id, topic.num_subscribers, topic.num_publishers);

    // Remove client from list and shift all the clients after him one position left.
    // Remove one from topic counter.
    if (action == UNREGISTER_SUBSCRIBER){
        shift_client_list(topic, action, id);
        topic.num_subscribers--;
        next_subscriber--; // So that we assign the correct id to the next registered subscriber
    } else if (action == UNREGISTER_PUBLISHER){
        topic.num_publishers--;
        next_publisher--; // So that we assign the correct id to the next registered publisher
    }

    DEBUG_PRINTF("UNREGISTER TOPIC: %s. Topic ID: %d. Subs: %d, Pubs: %d\n",
     topic.name, topic_id, topic.num_subscribers, topic.num_publishers);

}

void send_sequential(struct message message){
    DEBUG_PRINTF("SENDING SEQUENTIAL\n");
    // SECUENCIAL
    struct publish publish;
    int socket, topic_id, num_subs, i;   

    strcpy(publish.data, message.data.data);
    publish.time_generated_data = message.data.time_generated_data;
    topic_id = get_topic_id(message.topic);
    num_subs = all_topics[topic_id].num_subscribers;
    for(i = 0; i < num_subs; i++){
        socket = all_topics[topic_id].subscribers[i].fd;
        DEBUG_PRINTF("MSG SENT. FD: %d\n", socket);
        if (send(socket, (void *)&publish, sizeof(publish), 0) < 0){
            warnx("send() failed. %s\n", strerror(errno));
            exit(1);
        }
    }
}

void* send_parallel(void *ptr){
    DEBUG_PRINTF("SENDING PARALLEL\n");
    struct send_message *ptr_msg = (struct send_message*)ptr;
    struct send_message send_message = *ptr_msg;

    DEBUG_PRINTF("FD: %d\n", send_message.fd);

    if (send(send_message.fd, (void *)&send_message.message, sizeof(send_message.message), 0) < 0){
         warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    pthread_exit(NULL);
}

void* send_just(void *ptr){
    struct send_message *ptr_msg = (struct send_message*)ptr;
    struct send_message send_message = *ptr_msg;
    int topic_id = get_topic_id(send_message.message.topic);

    // All the threads will wait for the signal
    // The number of threads to join is equal to the number of subscribers.
    // The last thread with signal the rest to continue.

    pthread_mutex_lock(&mutex);
    just_threads++;
    
    DEBUG_PRINTF("just_threads, %d, subs: %d\n", just_threads, all_topics[topic_id].num_subscribers);
    if (just_threads == all_topics[topic_id].num_subscribers - 1){
        pthread_cond_signal(&just_condition);
        pthread_mutex_unlock(&mutex);
        just_threads = 0;    
    }

    DEBUG_PRINTF("WAITING... FD: %d\n",send_message.fd);
    pthread_cond_wait(&just_condition, &mutex);
    // Release the mutex immediatly so the next thread can join.
    // There are no race conditions, so its okay.
    pthread_cond_signal(&just_condition);
    pthread_mutex_unlock(&mutex); 

    DEBUG_PRINTF("SENDING JUST\n");
    if (send(send_message.fd, (void *)&send_message.message, sizeof(send_message.message), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }
    
    pthread_exit(NULL);
}

void publish_msg(struct message message){
    DEBUG_PRINTF("MODE: %d\n", mode);
    if (mode == SECUENCIAL){
        send_sequential(message);

    } else if (mode == PARALELO) {

        int topic_id = get_topic_id(message.topic);
        int num_subscribers = all_topics[topic_id].num_subscribers;
        int i;
        for(i = 0; i < num_subscribers; i++){
            struct send_message *send_message = malloc(256);
            send_message->message = message;
            send_message->id = i;
            send_message->fd = all_topics[topic_id].subscribers[i].fd;
            pthread_t new_thread;
            if (pthread_create(&new_thread, NULL, send_parallel, (void *) send_message) < 0){
                warnx("Error while creating Thread\n");
                exit(EXIT_FAILURE);
            }
        } 
        

    } else if (mode == JUSTO){
        int topic_id = get_topic_id(message.topic);
        int num_subscribers = all_topics[topic_id].num_subscribers;
        int i;
        pthread_t threads[num_subscribers];
        for(i = 0; i < num_subscribers; i++){
            struct send_message *send_message = malloc(256);
            send_message->message = message;
            send_message->id = i;
            send_message->fd = all_topics[topic_id].subscribers[i].fd;
            if (pthread_create(&threads[i], NULL, send_just, (void *) send_message) < 0){
                warnx("Error while creating Thread\n");
                exit(EXIT_FAILURE);
            }
        } 
        for(i = 0; i < num_subscribers; i++){
            pthread_join(threads[i], NULL);
        }
    }
}

void talk_to_publisher(int client_socket_){
    int action, id;
    char *data;
    struct message message;
    struct timespec time_generated_data, time_received_data;
    long gen_sec, gen_nsec, recv_sec, recv_nsec;

    DEBUG_PRINTF("TALKING TO PUBLISHER\n");
    // Wait for a message from the publisher.
    // Then check topic and send message to all subscribers.
    while(1){
        if (recv(client_socket_, (void *)&message, sizeof(message), 0) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            exit(1);
        }

        action = message.action;
        if(action == UNREGISTER_PUBLISHER){
            id = message.id;
            DEBUG_PRINTF("RECV: UNREGISTER %d: Topic: %s, ID: %d\n", action, message.topic, id);
            unregister(action, message.topic, id);
        } else if(action == PUBLISH_DATA){
            data = message.data.data;
            time_generated_data = message.data.time_generated_data;

            clock_gettime(CLOCK_MONOTONIC, &time_received_data);

            gen_sec = time_generated_data.tv_sec;
            gen_nsec = time_generated_data.tv_nsec;
            recv_sec = time_received_data.tv_sec;
            recv_nsec = time_received_data.tv_nsec;

            printf("[%ld.%ld] Recibido mensaje para publicar en topic: %s - mensaje: %s - Generó %ld.%ld\n", 
            recv_sec, recv_nsec, message.topic, data, gen_sec, gen_nsec);

            publish_msg(message);
        } 
    }
}

void talk_to_subscriber(int client_socket_){
    int action;
    DEBUG_PRINTF("TALKING TO SUBSCRIBER\n");
    struct message message;
    if (recv(client_socket_, (void *)&message, sizeof(message), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        exit(1);
    }

    action = message.action;
    if(action == UNREGISTER_SUBSCRIBER){
        char *topic = message.topic; 
        int id = message.id;
        DEBUG_PRINTF("RECV: UNREGISTER %d: Topic: %s, ID: %d\n", action, topic, id);
        unregister(action, topic, id);
    } 
}

void init_broker(char* ip, int port, char* mode_){

    //DEBUG_PRINTF("Server create socket\n");
    server_socket = socket_create();

    //DEBUG_PRINTF("Server set IP and Port\n");
    set_ip_port(ip,port);

    //DEBUG_PRINTF("Server binding\n");
    socket_bind(server_socket);

    //DEBUG_PRINTF("Server listening\n");
    socket_listen(server_socket);

    if (strcmp(mode_, "secuencial") == 0 ){
        mode = SECUENCIAL;
    } else if (strcmp(mode_, "paralelo") == 0 ){
        mode = PARALELO;
    } else if (strcmp(mode_, "justo") == 0 ){
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
                            
                // Serach for the requested topic.                            
                // Add publisher to current topic's publishers list.
                // topic_id identifies de position of the topic in the all_topics list.
                int topic_id = get_topic_id(message.topic);
                int topic_subscribers = all_topics[topic_id].num_subscribers;
                all_topics[topic_id].subscribers[topic_subscribers] = new_subscriber;

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
        int topic_id;
        int topic_exists = 0;
        for(topic_id = 0; topic_id < current_topics; topic_id++){
            if (strcmp(all_topics[topic_id].name, message.topic) == 0){
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

                // Add publisher to current topic's publishers list.
                // topic_id was obtained when we checked if the topic existed.
                // It identifies de position of the topic in the all_topics list.
                int topic_publishers = all_topics[topic_id].num_publishers;
                all_topics[topic_id].publishers[topic_publishers] = new_publisher;

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
                    all_topics[i].num_publishers++; // Increase current topic publishers counter
                }  else {
                    all_topics[i].num_subscribers++;// Increase current topic subscribers counter
                }
            }

            printf("\t%s: %d Suscriptores - %d Publicadores\n",
            all_topics[i].name, all_topics[i].num_subscribers, all_topics[i].num_publishers);
        }
    }

    // Listen to client and communicate.
    if ( (message.action == REGISTER_PUBLISHER) && (response.id != -1)){
        talk_to_publisher(client_socket_);
    } else if ( (message.action == REGISTER_SUBSCRIBER) && (response.id != -1)){
        talk_to_subscriber(client_socket_);
    }

    pthread_exit(NULL);  
};