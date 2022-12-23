// Gonzalo Vega Pérez - 2022

#include "proxy_broker.h"
struct sockaddr_in addr, client_addr_;
char proc_name[32];

int server_socket, mode, next_publisher_id, next_subscriber_id = 1,
current_publishers, current_subscribers, current_topics,
next_pub_position, next_sub_position;

struct topic all_topics[MAX_TOPICS];

int just_threads;
sigset_t just_signal;

pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_mutex_t mutex3;
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

void shift_client_list(struct topic *topic, int action, int id){
    DEBUG_PRINTF("SHIFTING LIST\n");
    int client_pos;
    int i;
    int fd_to_del;
    struct response response;
    // Subscriber ID will be its position in the array + 1;
    // Publisher ID will be its position in the array.

    // Every client from the one we want to delete to the last one will be
    // replaced by the one after it.
    // If we have [1, 2, 3, 4, 0] and we want to delete 2, we do:
    // [1,2,3,4,0] -> [1,3,3,4,0] -> [1,3,4,0,0]

    if (action == UNREGISTER_SUBSCRIBER){
        
        // DEBUG //
        for (int j = 0; j < 10; j++){
            DEBUG_PRINTF("Position %d: ID: %d FD: %d\n",
            j, topic->subscribers[j].id ,
            topic->subscribers[j].fd);
        }
        ///////////

        for(client_pos = 0; client_pos < MAX_SUBSCRIBERS; client_pos++){
            // If the ids matches, we found our client.
            if(topic->subscribers[client_pos].id == id){
                fd_to_del = topic->subscribers[client_pos].fd;
                break;
            }  
        }

        DEBUG_PRINTF("REMOVING SUBSCRIBER. ID: %d, FD: %d POS: %d\n",
         id, fd_to_del, client_pos);

        for(i = client_pos; i < MAX_SUBSCRIBERS; i++){
            topic->subscribers[i] = topic->subscribers[i+1];  
        }
        
        // DEBUG //
        for (int j = 0; j < 10; j++){
            DEBUG_PRINTF("Position %d: ID: %d FD: %d\n",
            j, topic->subscribers[j].id ,
            topic->subscribers[j].fd);
        }
        ///////////

    } else if (action == UNREGISTER_PUBLISHER){

        // DEBUG //
        for (int j = 0; j < 10; j++){
            DEBUG_PRINTF("Position %d: ID: %d FD: %d\n",
            j, topic->publishers[j].id ,
            topic->publishers[j].fd);
        }
        ///////////

        for(client_pos = 0; client_pos < MAX_PUBLISHERS; client_pos++){
            // If the ids matches, we found our client.
            if(topic->publishers[client_pos].id == id){
                fd_to_del = topic->publishers[client_pos].fd;
                break;
            }  
        }
        
        DEBUG_PRINTF("REMOVING PUBLISHER. ID: %d, FD: %d POS: %d\n",
         id, fd_to_del, client_pos);

        for(i = client_pos; i < MAX_PUBLISHERS; i++){
            topic->publishers[i] = topic->publishers[i+1];  
        }

        // DEBUG //
        for (int j = 0; j < 10; j++){
            DEBUG_PRINTF("Position %d: ID: %d FD: %d\n",
            j, topic->publishers[j].id ,
            topic->publishers[j].fd);
        }
        ///////////

    }
    response.id = id;
    response.response_status = OK;
    if (send(fd_to_del, (void *)&response, sizeof(response), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }
    // FILE DESCRIPTOR WILL BE CLOSED BY CLIENT.
}

void unregister(int action, char *topic_name, int id){
    struct timespec time;
    long time_seconds;
    long time_nanoseconds;

    char client_type[16]; // For printing
    DEBUG_PRINTF("UNREGISTER %d: Topic: %s, ID: %d\n", action, topic_name, id);
    int topic_id = get_topic_id(topic_name);

    // Remove client from list and shift all the clients after him one position left.
    // Remove one from topic counter.
    if (action == UNREGISTER_SUBSCRIBER){
        strcpy(client_type, "Suscriptor");
        shift_client_list(&all_topics[topic_id], action, id);
        all_topics[topic_id].num_subscribers--;
        next_sub_position--;
    } else if (action == UNREGISTER_PUBLISHER){
        strcpy(client_type, "Publicador");
        shift_client_list(&all_topics[topic_id], action, id);
        all_topics[topic_id].num_publishers--;
        next_pub_position--;
    }

    DEBUG_PRINTF("TOPIC NOW: %s. Topic ID: %d. Subs: %d, Pubs: %d\n",
     all_topics[topic_id].name, topic_id, 
     all_topics[topic_id].num_subscribers, all_topics[topic_id].num_publishers);

    clock_gettime(CLOCK_REALTIME, &time);
    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;

    printf("[%ld.%ld] Eliminado Cliente (%d) %s: %s\n Resumen:\n",
    time_seconds, time_nanoseconds, id, client_type, topic_name);
    int i;
    for(i = 0; i < current_topics; i++){
        printf("\t%s: %d Suscriptores - %d Publicadores\n",
        all_topics[i].name, all_topics[i].num_subscribers, all_topics[i].num_publishers);
    }

    // As the client disconnected, we don't need this thread anymore.
    pthread_exit(NULL); 
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
    struct publish publish;
    DEBUG_PRINTF("SENDING PARALLEL\n");
    struct send_message *ptr_msg = (struct send_message*)ptr;
    struct send_message send_message = *ptr_msg;

    DEBUG_PRINTF("FD: %d\n", send_message.fd);

    strcpy(publish.data, send_message.message.data.data);
    publish.time_generated_data = send_message.message.data.time_generated_data;

    if (send(send_message.fd, (void *)&publish, sizeof(publish), 0) < 0){
         warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    pthread_exit(NULL);
}

void* send_just(void *ptr){
    struct send_message *ptr_msg = (struct send_message*)ptr;
    struct send_message send_message = *ptr_msg;
    int topic_id = get_topic_id(send_message.message.topic);
    struct publish publish;

    strcpy(publish.data, send_message.message.data.data);
    publish.time_generated_data = send_message.message.data.time_generated_data;

    // All the threads will wait for the signal
    // The number of threads to join is equal to the number of subscribers.
    // The last thread will pass and signal the rest to continue.

    pthread_mutex_lock(&mutex);
    just_threads++;
    
    DEBUG_PRINTF("just_threads, %d, subs: %d FD: %d\n", 
    just_threads, all_topics[topic_id].num_subscribers, send_message.fd);

    // All threads except the last one will wait.
    if (just_threads < all_topics[topic_id].num_subscribers){
        DEBUG_PRINTF("WAITING... FD: %d\n",send_message.fd);
        pthread_cond_wait(&just_condition, &mutex);
    }
    just_threads--;
    // Release the mutex immediatly so the next thread can join.
    // There are no race conditions, so its okay.
    pthread_cond_broadcast(&just_condition);
    pthread_mutex_unlock(&mutex); 

    DEBUG_PRINTF("SENDING JUST. FD: %d\n", send_message.fd);
    if (send(send_message.fd, (void *)&publish, 
    sizeof(publish), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }
    
    pthread_exit(NULL);
}

void publish_msg(struct message message){
    struct timespec time;
    long time_seconds;
    long time_nanoseconds;
    int topic_id = get_topic_id(message.topic);
    int num_subscribers = all_topics[topic_id].num_subscribers;

    //.//.//.//.//.//. PRINT MESSAGE //.//.//.//.//.//.
    clock_gettime(CLOCK_REALTIME, &time);
    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;

    printf("[%ld.%ld] Enviando mensaje en topic %s a %d suscriptores\n",
    time_seconds, time_nanoseconds, message.topic, num_subscribers);
    //.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.//.

    if (mode == SECUENCIAL){
        send_sequential(message);

    } else if (mode == PARALELO) {
        int i;
        for(i = 0; i < num_subscribers; i++){
            struct send_message *send_message = malloc(256);
            send_message->message = message;
            send_message->fd = all_topics[topic_id].subscribers[i].fd;
            pthread_t new_thread;
            if (pthread_create(&new_thread, NULL, send_parallel, (void *) send_message) < 0){
                warnx("Error while creating Thread\n");
                exit(EXIT_FAILURE);
            }
        } 
        

    } else if (mode == JUSTO){
        // This mutex2 protects the variable just_threads used in send_just()
        // if two threads want to publish at the same time.

        // A thread will create as many threads as subscribers in the topic.
        // We want only the threads created by this one to modify just_threads.
        // We don't want the threads created by another one to modify it at the same time.
        pthread_mutex_lock(&mutex2);
        int i;
        pthread_t threads[num_subscribers];
        for(i = 0; i < num_subscribers; i++){
            struct send_message *send_message = malloc(256);
            send_message->message = message;
            send_message->fd = all_topics[topic_id].subscribers[i].fd;
            if (pthread_create(&threads[i], NULL, send_just, (void *) send_message) < 0){
                warnx("Error while creating Thread\n");
                exit(EXIT_FAILURE);
            }
        } 
        for(i = 0; i < num_subscribers; i++){
            pthread_join(threads[i], NULL);
        }
        pthread_mutex_unlock(&mutex2);
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

            clock_gettime(CLOCK_REALTIME, &time_received_data);

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

    pthread_mutex_lock(&mutex3); // Prevents threads to modify a topic at the same time

    DEBUG_PRINTF("New message: action: %d, topic: %s\n",message.action, message.topic);

    // Get time when the message was recieved for printing
    clock_gettime(CLOCK_REALTIME, &time);
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
                new_subscriber.id = next_subscriber_id;                // Fill the ID of this subscriber.
                            
                // Serach for the requested topic.                            
                // Add publisher to current topic's publishers list.
                // topic_id identifies de position of the topic in the all_topics list.
                int topic_id = get_topic_id(message.topic);
                int topic_subscribers = all_topics[topic_id].num_subscribers;
                all_topics[topic_id].subscribers[topic_subscribers] = new_subscriber;

                // Fill variables for printing message.
                id = next_subscriber_id;
                client_type = "Suscriptor";
                topic = new_subscriber.topic;

                // Fill response message variables.
                response.id = next_subscriber_id++;
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
            if (next_publisher_id < MAX_PUBLISHERS){
                // Create new publisher structure
                struct publisher new_publisher;
                new_publisher.fd = client_socket_;              // Fill the FD used to communicate with this publisher
                strcpy(new_publisher.topic, message.topic);     // Fill the topic this publisher is writing to.
                new_publisher.id = next_publisher_id;              // Fill the ID of this publisher.

                // Add publisher to current topic's publishers list.
                // topic_id was obtained when we checked if the topic existed.
                // It identifies de position of the topic in the all_topics list.
                int topic_publishers = all_topics[topic_id].num_publishers;
                all_topics[topic_id].publishers[topic_publishers] = new_publisher; // add at the end of the list.

                // Fill variables for printing message.
                id = next_publisher_id;
                client_type = "Publicador";
                topic = new_publisher.topic;

                // Fill response message variables.
                response.id = next_publisher_id++;
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

    pthread_mutex_unlock(&mutex3);

    // Listen to client and communicate.
    if ( (message.action == REGISTER_PUBLISHER) && (response.id != -1)){
        talk_to_publisher(client_socket_);
    } else if ( (message.action == REGISTER_SUBSCRIBER) && (response.id != -1)){
        talk_to_subscriber(client_socket_);
    }

    pthread_exit(NULL);  
};