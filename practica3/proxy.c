// Gonzalo Vega Pérez - 2022

#include "proxy.h"
pthread_mutex_t mutex;
pthread_mutex_t counter_mutex;
pthread_mutex_t current_threads_mutex;
struct sockaddr_in addr, client_addr_;
char proc_name[6];
int counter = 0;
int current_readers = 0;
int current_writers = 0;
int current_threads = 0;
int priority;

void set_priority(char *prio){
    if (strcmp(prio, "reader") == 0){
        priority = READ;
        DEBUG_PRINTF("PRIORITY SET TO READ\n");
    } else {
        priority = WRITE;
        DEBUG_PRINTF("PRIORITY SET TO WRITE\n");
    }


}

void set_current_threads (){
    pthread_mutex_lock(&current_threads_mutex);
    current_threads++;
    pthread_mutex_unlock(&current_threads_mutex);
};

int get_current_threads (){
    return current_threads;
};

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

// // // // // // SERVER ONLY // // // // // //

struct request receive_request(int client_socket_){
    struct request recv_msg;

    if (recv(client_socket_, (void *)&recv_msg, sizeof(recv_msg), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        exit(1);
    }  

    DEBUG_PRINTF("RECV: action: %d | id: %d\n", recv_msg.action, recv_msg.id);

    return recv_msg;
}

void send_response(struct response response, int client_socket_){

    if (send(client_socket_, (void *)&response, sizeof(response), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }
}

struct response do_request(struct request request){
    int locked = 0;
    char *action; 
    char *verb;
    struct response response;
    struct timespec sleep_time;
    struct timespec wait_time_start;
    struct timespec wait_time_end;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = rand() % 75000000 + 75000000; // Random number between 75000000 and 150000000.


    if (clock_gettime(CLOCK_MONOTONIC, &wait_time_start) != 0){
        warnx("clock_gettime() failed. %s\n",strerror(errno));
        exit(1);
    }
    // // // // REGIÓN CRÍTICA // // // //
    if (current_writers > 0){
        pthread_mutex_lock(&mutex);
        locked = 1;
    } 
   
    DEBUG_PRINTF("LOCK\n");
    if (clock_gettime(CLOCK_MONOTONIC, &wait_time_end) != 0){
        warnx("clock_gettime() failed. %s\n",strerror(errno));
        exit(1);
    }

    if (request.action == WRITE){
        counter++;
        action = "ESCRITOR";
        verb = "modifica";
    } else {
        action = "LECTOR";
        verb = "lee";
    }

    // Sleep. (Task leaves CPU)
    DEBUG_PRINTF("Sleeping for %ld ms\n",sleep_time.tv_nsec/1000000);
    if (nanosleep(&sleep_time, NULL) < 0){
        warnx("nanosleep() failed. %s\n",strerror(errno));
        pthread_mutex_unlock(&mutex);
        exit(1);
    }

    printf("[%ld] [%s #%d] %s contador con valor %d\n",
    sleep_time.tv_nsec/1000, action, request.id, verb, counter);

    write_output();



    pthread_mutex_lock(&counter_mutex);
    
    if (request.action == READ){
        current_readers--;
    } else {
        current_writers--;
    }

    pthread_mutex_unlock(&counter_mutex);

    if (locked){
        DEBUG_PRINTF("UNLOCK\n");
        pthread_mutex_unlock(&mutex);
    }
    // // // // // // // // // // // //

    response.action = request.action;
    response.counter = counter;
    int waited_sec = wait_time_end.tv_sec - wait_time_start.tv_sec;
    long waited_nsec = wait_time_end.tv_nsec - wait_time_start.tv_nsec;
    long waited_total_ns =  waited_nsec + waited_sec*1000000000;
    response.waiting_time = waited_total_ns;

    return response;
}


void close_client_socket(int client_socket_){
    close(client_socket_);
}

void write_output(){
    FILE *fpt;
    fpt = fopen("server_output.txt",  "w");
    fprintf(fpt,"%d",counter);
    fclose(fpt);
}

void read_output(){
    char buffer[20];
    FILE *fpt;
    fpt = fopen("server_output.txt",  "r");
    while (fgets(buffer, sizeof(buffer), fpt) != NULL) {}
    fclose(fpt);
    counter = atoi(buffer);
}

void *talk_2_client(void *ptr){
    int client_socket_ = *(int*)ptr;

    DEBUG_PRINTF("Thread receiving. Client_socket: %d\n", client_socket_);
    struct request request = receive_request(client_socket_);

    pthread_mutex_lock(&counter_mutex);
    
    if (request.action == READ){
        current_readers++;
    } else {
        current_writers++;
    }

    pthread_mutex_unlock(&counter_mutex);

    DEBUG_PRINTF("Thread doing request Client_socket: %d\n", client_socket_);
    struct response response = do_request(request);

    DEBUG_PRINTF("Thread Sending response Client_socket: %d\n", client_socket_);
    send_response(response, client_socket_);

    pthread_mutex_lock(&current_threads_mutex);
        current_threads--;
    pthread_mutex_unlock(&current_threads_mutex);

    pthread_exit(NULL);
}
// // // // // // // // // // // // // // // //


// // // // // // CLIENT ONLY // // // // // //

void *talk_2_server(void *ptr){

    char *response_mode;
    struct response response;
    struct request request;
    struct client_threads *client_threads_ = ((struct client_threads *)ptr);
    int socket_ =  client_threads_->socket;
    char *mode = client_threads_->mode;
    int thread_id = client_threads_->thread_id;
    enum operations action;
    action = WRITE;

    //DEBUG_PRINTF("Client %d connecting... (%s)\n",thread_id, mode);
    socket_connect(socket_);

    if (strcmp(mode, "reader") == 0){
        action = READ;
    }
    
    request.action = action;
    request.id = thread_id;
    DEBUG_PRINTF("SENT: action: %d, id: %d\n", request.action, request.id);

    if (send(socket_, (void *)&request, sizeof(request), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    response = receive_response(socket_);

    if (response.action == READ){
        response_mode = "Lector";
    } else {
        response_mode = "Escritor";   
    }
    
    printf("[Cliente #%d] %s, contador=%d, tiempo=%ld ns\n",
     thread_id, response_mode, response.counter, response.waiting_time);

    pthread_exit(NULL);
};


struct response receive_response(int socket_){
    struct response recv_msg;

    if (recv(socket_, (void *)&recv_msg, sizeof(recv_msg), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        close(socket_);
        exit(1);
    }  

    return recv_msg;
}

void close_client(int socket_){
    close(socket_);
}
// // // // // // // // // // // // // // // //