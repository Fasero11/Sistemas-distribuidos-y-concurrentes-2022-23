// Gonzalo Vega Pérez - 2022

#include "proxy.h"
pthread_mutex_t mutex;
pthread_mutex_t counter_mutex;
pthread_mutex_t current_threads_mutex;
pthread_mutex_t free_fd_mutex;
pthread_mutex_t ratio_mutex;
pthread_mutex_t writers_mutex;
pthread_mutex_t readers_mutex;
pthread_cond_t not_full;
pthread_cond_t allow_writers;
pthread_cond_t allow_readers;
struct sockaddr_in addr, client_addr_;
char proc_name[6];
int client_sockets[MAX_THREADS]; // Initialized to zero
int counter = 0;
int current_readers = 0;
int current_writers = 0;
int current_threads = 0;
int priority;
int ratio;
int ratio_counter;
int server_socket;

void init_server(char* ip, int port, char* priority_, int ratio_){

    //DEBUG_PRINTF("Server create socket\n");
    server_socket = socket_create();

    //DEBUG_PRINTF("Server set IP and Port\n");
    set_ip_port(ip,port);

    //DEBUG_PRINTF("Server binding\n");
    socket_bind(server_socket);

    //DEBUG_PRINTF("Server listening\n");
    socket_listen(server_socket);

    set_priority(priority_);

    set_ratio(ratio_);
}

void init_server_thread(int *thread_info){

    int client_socket_ = socket_accept(server_socket);
    int id = get_free_fd();
    set_value_fd (id, client_socket_);


    thread_info[0] = id;
    thread_info[1] = client_socket_;
}

void set_ratio(int ratio_){
    ratio = ratio_;
};

void set_counter(int counter_){
    counter = counter_;
};

void set_priority(char *prio){
    if (strcmp(prio, "reader") == 0){
        priority = READ;
        DEBUG_PRINTF("PRIORITY SET TO READ\n");
    } else {
        priority = WRITE;
        DEBUG_PRINTF("PRIORITY SET TO WRITE\n");
    }
}

void set_current_threads (int value){
    pthread_mutex_lock(&current_threads_mutex);
    current_threads = current_threads + value;

    if (value < 0){
        pthread_cond_signal(&not_full);
    }
    pthread_mutex_unlock(&current_threads_mutex);
};

int get_current_threads (){
    return current_threads;
};

int get_free_fd (){
    // Returns the first free position in the array.
    // 0 = free; Otherwise = in use;
    pthread_mutex_lock(&free_fd_mutex);
    
    if (current_threads >= MAX_THREADS){
        //DEBUG_PRINTF("MAX THREADS REACHED: %d\n", get_current_threads());
        pthread_cond_wait(&not_full, &free_fd_mutex);
        //DEBUG_PRINTF("MAX THREADS FREE: %d\n", get_current_threads());
    }

    int id = 0;
    while (id < MAX_THREADS){
        //DEBUG_PRINTF("CLIENT_SOCKET %d, value: %d\n",id, client_sockets[id]);
        if (client_sockets[id] == 0){
            client_sockets[id] = 1; // Set as in use
            break;
        }
        id++;
    }
    pthread_mutex_unlock(&free_fd_mutex);
    //DEBUG_PRINTF("RETURNING ID: %d\n",id);
    return id;
}

void set_value_fd (int id, int value){
    pthread_mutex_lock(&free_fd_mutex);
    client_sockets[id] = value;
    pthread_mutex_unlock(&free_fd_mutex);
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

struct request receive_request(int client_socket_){
    struct request recv_msg;

    if (recv(client_socket_, (void *)&recv_msg, sizeof(recv_msg), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        exit(1);
    }  

    DEBUG_PRINTF("RECV: action: %d | id: %d\n", recv_msg.action, recv_msg.id);

    return recv_msg;
}

void send_response(struct response response, int socket){

    if (send(socket, (void *)&response, sizeof(response), 0) < 0){
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

    pthread_mutex_lock(&counter_mutex);
    if (request.action == READ){
        current_readers++;
    } else {
        current_writers++;
    }

    if (request.action == READ && (current_writers > 1 || current_readers > 1) && ratio != 0){
        DEBUG_PRINTF("READER WAITING\n");
        pthread_mutex_lock(&readers_mutex);
        pthread_mutex_unlock(&counter_mutex);
        DEBUG_PRINTF("READER CONDITION WAITING\n");
        pthread_cond_wait(&allow_readers, &readers_mutex);
        pthread_mutex_unlock(&readers_mutex);    
    }

    else if (request.action == WRITE && (current_writers > 1 || current_readers > 1) && ratio != 0){
        DEBUG_PRINTF("WRITER WAITING\n");
        pthread_mutex_lock(&writers_mutex);
        pthread_mutex_unlock(&counter_mutex);
        DEBUG_PRINTF("WRITER CONDITION WAITING\n");
        pthread_cond_wait(&allow_writers, &writers_mutex);
        pthread_mutex_unlock(&writers_mutex);      
          
    } else {
        pthread_mutex_unlock(&counter_mutex);
    }

    // Take mutex if you are a reader without priority and there are writers or if you are a writer.
    if ((current_writers > 0 && request.action == READ) || (request.action == WRITE)){
        DEBUG_PRINTF("LOCK: %d | current_writers: %d | current_readers: %d\n", request.action,current_writers,current_writers);
        pthread_mutex_lock(&mutex);
        locked = 1;
    }

    // // // // REGIÓN CRÍTICA // // // //

    if (request.action == priority){
        pthread_mutex_lock(&ratio_mutex);
        ratio_counter++;   // Increase by 1 if a prio client has entered.
        pthread_mutex_unlock(&ratio_mutex);
    } else {
        ratio_counter = 0;
    }

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
    //DEBUG_PRINTF("Sleeping for %ld ms\n",sleep_time.tv_nsec/1000000);
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

    DEBUG_PRINTF("R: %d | RATIO_COUNTER: %d\n",ratio, ratio_counter);

    if (ratio_counter >= ratio && ratio != 0 && request.action == READ && current_writers > 0) {
        ratio_counter = 0;
        DEBUG_PRINTF("RATIO ALLOW WRITERS\n");
        pthread_mutex_lock(&ratio_mutex);
        pthread_cond_signal(&allow_writers);
        pthread_mutex_unlock(&ratio_mutex);
    } else if (ratio_counter >= ratio && ratio != 0 && request.action == WRITE && current_readers > 0){
        ratio_counter = 0;
        DEBUG_PRINTF("RATIO ALLOW WRITERS\n");
        pthread_mutex_lock(&ratio_mutex);
        pthread_cond_signal(&allow_readers);
        pthread_mutex_unlock(&ratio_mutex);

    } else if (priority == READ && current_readers > 0){
        DEBUG_PRINTF("NEXT_READER PRIORITY ALLOWED\n");
        pthread_cond_signal(&allow_readers);
    } else if (priority == WRITE && current_writers > 0){
        DEBUG_PRINTF("NEXT_WRITER PRIORITY ALLOWED\n");
        pthread_cond_signal(&allow_writers);
    } else if (request.action == READ && current_readers > 0){
        DEBUG_PRINTF("NEXT_READER 1\n");
        pthread_cond_signal(&allow_readers);
    } else if (request.action == WRITE && current_writers > 0){
        DEBUG_PRINTF("NEXT_WRITER 1\n");
        pthread_cond_signal(&allow_writers);
    } else if (request.action == READ && current_readers == 0){
        DEBUG_PRINTF("NEXT_WRITER 2\n");
        pthread_cond_signal(&allow_writers);
    } else if (request.action == WRITE && current_writers == 0){
        DEBUG_PRINTF("NEXT_READER 2\n");
        pthread_cond_signal(&allow_readers);
    } else {
        DEBUG_PRINTF("DEFAULT\n");
        pthread_cond_signal(&allow_readers);
    }

    pthread_mutex_unlock(&counter_mutex);

    DEBUG_PRINTF("UNLOCK\n");
    if (locked){
        pthread_mutex_unlock(&mutex);
    }
    // // // // // // // // // // // //

    response.action = request.action;
    response.counter = counter;
    long waited_sec = wait_time_end.tv_sec - wait_time_start.tv_sec;
    long waited_nsec = wait_time_end.tv_nsec - wait_time_start.tv_nsec;
    long waited_total_ns =  waited_nsec + waited_sec*1000000000;
    //DEBUG_PRINTF("SECS: %ld, NSEC: %ld\n",waited_sec, waited_nsec);
    response.waiting_time = waited_total_ns;

    return response;
}

void write_output(){
    FILE *fpt;
    fpt = fopen("server_output.txt",  "w");
    fprintf(fpt,"%d",counter);
    fclose(fpt);
}

void *talk_2_client(void *ptr){
    int *values = (int*)ptr;

    int id = values[0];
    int client_socket_ = values[1];

    //DEBUG_PRINTF("Thread receiving. Client_socket: %d, ID: %d\n", client_socket_, id);
    struct request request = receive_request(client_socket_);

    //DEBUG_PRINTF("Thread doing request Client_socket: %d\n", client_socket_);
    struct response response = do_request(request);

    //DEBUG_PRINTF("Thread Sending response Client_socket: %d\n", client_socket_);
    send_response(response, client_socket_);

    close(client_sockets[id]);
    set_value_fd (id, 0);
    set_current_threads(-1);

    //DEBUG_PRINTF("ID %d NOW FREE. | CURRENT_THREADS: %d\n", id, get_current_threads());

    free(values);
    pthread_exit(NULL);
}

void *talk_2_server(void *ptr){

    char *response_mode;
    struct response response;
    struct request request;
    struct client_threads *client_threads_ = ((struct client_threads *)ptr);
    char *mode = client_threads_->mode;
    int thread_id = client_threads_->thread_id;
    enum operations action;
    action = WRITE;

    //DEBUG_PRINTF("Client %d connecting... (%s)\n",thread_id, mode);
    int socket_ =  socket_create();
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

    DEBUG_PRINTF("Thread %d waiting response\n", thread_id);
    response = receive_response(socket_);

    if (response.action == READ){
        response_mode = "Lector";
    } else {
        response_mode = "Escritor";   
    }
    
    printf("[Cliente #%d] %s, contador=%d, tiempo=%ld ns\n",
     thread_id, response_mode, response.counter, response.waiting_time);

    close(socket_);

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