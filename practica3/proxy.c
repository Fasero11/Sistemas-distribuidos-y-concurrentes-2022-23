// Gonzalo Vega Pérez - 2022

#include "proxy.h"
pthread_mutex_t mutex;
struct sockaddr_in addr, client_addr_;
char proc_name[6];
int socket_, client_socket_, counter = 0;

void set_name (char name[6]){
    strcpy(proc_name,name);
};

void set_ip_port (char* ip, unsigned int port){
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    DEBUG_PRINTF("%s: IP set: %s | Port set: %d \n",proc_name ,ip, port);
};

void socket_create(){
    setbuf(stdout, NULL);
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0){
        warnx("%s: Error creating socket. %s\n",proc_name ,strerror(errno));
        exit(1); 
    }
    DEBUG_PRINTF("%s: Socket successfully created...\n",proc_name );
};

void socket_connect(){
    while (connect(socket_, (struct sockaddr *)&addr, sizeof(addr)) != 0){
        DEBUG_PRINTF("%s: Server not found\n",proc_name );
    }
    DEBUG_PRINTF ("%s: Connected to server...\n",proc_name );
};

void socket_bind(){
    if (bind(socket_, (struct sockaddr*)&addr, 
    sizeof(addr)) < 0){
        warnx("%s: bind() failed. %s\n",proc_name , strerror(errno));
        close(socket_);
        exit(1); 
    }
    DEBUG_PRINTF("%s: Socket successfully binded...\n",proc_name );
    DEBUG_PRINTF("%s: Binded to port: %d\n",proc_name , addr.sin_port);
};

void socket_listen(){
    DEBUG_PRINTF("%s: Socket listening...\n",proc_name );
    if (listen(socket_, 5) < 0){
        warnx("%s: listen() failed. %s\n",proc_name , strerror(errno));
        close(socket_);
        exit(1);        
    }
};

void socket_accept(){
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

};

// // // // // // SERVER ONLY // // // // // //

struct request receive_request(){
    struct request recv_msg;

    if (recv(client_socket_, (void *)&recv_msg, sizeof(recv_msg), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        close(socket_);
        exit(1);
    }  

    DEBUG_PRINTF("RECV: action: %d | id: %d\n", recv_msg.action, recv_msg.id);

    return recv_msg;
}

void send_response(struct response response){

    if (send(client_socket_, (void *)&response, sizeof(response), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }
}

struct response do_request(struct request request){
    char* action;
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
    pthread_mutex_lock(&mutex);
    if (clock_gettime(CLOCK_MONOTONIC, &wait_time_end) != 0){
        warnx("clock_gettime() failed. %s\n",strerror(errno));
        exit(1);
    }

    if (request.action == WRITE){
        counter++;
        action = "ESCRITOR";
    } else {
        action = "LECTOR";
    }

    // Sleep. (Task leaves CPU)
    DEBUG_PRINTF("Sleeping for %ld ms\n",sleep_time.tv_nsec/1000000);
    if (nanosleep(&sleep_time, NULL) < 0){
        warnx("nanosleep() failed. %s\n",strerror(errno));
        exit(1);
    }

    printf("[%ld] [%s #%d] modifica contador con valor %d\n",
    sleep_time.tv_nsec/1000,action ,request.id, counter);

    write_output();
    pthread_mutex_unlock(&mutex);
    // // // // // // // // // // // //

    response.action = request.action;
    response.counter = counter;
    int waited_seconds = wait_time_end.tv_sec - wait_time_start.tv_sec;
    long waited_nsec = wait_time_end.tv_nsec - wait_time_start.tv_nsec;
    long waited_total_us = waited_seconds * 1000000 + waited_nsec / 1000;
    response.waiting_time = waited_total_us;

    return response;
}


void close_server(){
    close(client_socket_);
    close(socket_);
}

void write_output(){
    FILE *fpt;
    fpt = fopen("server_output.txt",  "a");
    fprintf(fpt,"%d",counter);
    fclose(fpt);
}
// // // // // // // // // // // // // // // //


// // // // // // CLIENT ONLY // // // // // //

void *talk_2_server(void *ptr){

    char *response_mode;
    struct response response;
    struct request request;
    struct client_threads *client_threads = ((struct client_threads *)ptr);
    char *mode = client_threads->mode;
    int thread_id = client_threads->thread_id;
    enum operations action;
    action = WRITE;

    DEBUG_PRINTF("Client %d connecting... (%s)\n",thread_id, mode);
    socket_connect();

    if (strcmp(mode, "reader") == 0){
        action = READ;
    }

    request.action = action;
    request.id = thread_id;
    DEBUG_PRINTF("action: %d, id: %d\n", request.action, request.id);

    if (send(socket_, (void *)&request, sizeof(request), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    response = receive_response();

    if (response.action == READ){
        response_mode = "Lector";
    } else {
        response_mode = "Escritor";   
    }
    
    printf("[Cliente #%d] %s, contador=%d, tiempo=%ld ns\n",
     thread_id, response_mode, response.counter, response.waiting_time);

    pthread_exit(NULL);
};


struct response receive_response(){
    struct response recv_msg;

    if (recv(socket_, (void *)&recv_msg, sizeof(recv_msg), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        close(socket_);
        exit(1);
    }  

    return recv_msg;
}

void close_client(){
    close(socket_);
}
// // // // // // // // // // // // // // // //