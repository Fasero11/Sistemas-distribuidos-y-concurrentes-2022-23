// Gonzalo Vega Pérez - 2022

#include "proxy_client.h"
struct sockaddr_in addr;
char proc_name[32];
int client_socket;
int mode;
char client_topic[100];
int client_id;
int client_type;

void sighandler(int signum){
    DEBUG_PRINTF("SIGNAL RECEIVED...  bye.\n");
    struct timespec time;
    long seconds, nanoseconds;

    struct message message;
    struct response response;

    if (client_type == PUBLISHER){
        message.action = UNREGISTER_PUBLISHER;
    } else {
        message.action = UNREGISTER_SUBSCRIBER;
    }

    strcpy(message.topic, client_topic);
    message.id = client_id;

    // Send Unregister until an OK response is received.
    do {
        if (send(client_socket, (void *)&message, sizeof(message), 0) < 0){
            warnx("send() failed. %s\n", strerror(errno));
            exit(1);
        }

        if (recv(client_socket, (void *)&response, sizeof(response), 0) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            exit(1);
        }
    } while (response.response_status != OK);

    clock_gettime(CLOCK_REALTIME, &time);
    seconds = time.tv_sec;
    nanoseconds = time.tv_nsec;

    printf("[%ld.%ld] De-Registrado (%d) correctamente del broker.\n",
    seconds, nanoseconds, client_id); 

    close(client_socket);
    exit(1);
}

void transfer_publisher(){
    DEBUG_PRINTF("Publishing...\n");  
    struct publish publish;
    struct message message;
    struct timespec time;
    long time_seconds, time_nanoseconds;

    message.action = PUBLISH_DATA;

    FILE* ptr;
    ptr = fopen("/proc/loadavg", "r");

    while(1){

        sleep(3);

        clock_gettime(CLOCK_REALTIME, &time);
        
        // Generate Data
        if (ptr != NULL){
            fread(publish.data, sizeof(char), 100, ptr);
        }

        DEBUG_PRINTF("DATA READ: %s\n",publish.data);

        time_seconds = time.tv_sec;
        time_nanoseconds = time.tv_nsec;

        publish.time_generated_data = time;
        message.data = publish;
        strcpy(message.topic, client_topic);

        printf("[%ld.%ld] Publicado mensaje topic: %s - mensaje: %s - Generó %ld.%ld\n",
        time_seconds, time_nanoseconds, client_topic, publish.data,
        time_seconds, time_nanoseconds);

        if (send(client_socket, (void *)&message, sizeof(message), 0) < 0){
            warnx("send() failed. %s\n", strerror(errno));
            exit(1);
        }
    }
}

void transfer_subscriber(){
    struct publish publish;
    struct timespec time;
    long received_seconds, received_nanoseconds,
    generated_seconds, generated_nanoseconds,
    latency_seconds, latency_nanoseconds;

    float latency;

    while(1){
        // Wait for data from the broker
        if (recv(client_socket, (void *)&publish, sizeof(publish), 0) < 0){
            warnx("recv() failed. %s\n", strerror(errno));
            exit(1);
        }

        clock_gettime(CLOCK_REALTIME, &time);
        received_seconds = time.tv_sec;
        received_nanoseconds = time.tv_nsec;
        generated_seconds = publish.time_generated_data.tv_sec;
        generated_nanoseconds = publish.time_generated_data.tv_nsec;

        if (received_nanoseconds - generated_nanoseconds < 0){
            latency_seconds = received_seconds - generated_seconds - 1;
            latency_nanoseconds = received_nanoseconds - generated_nanoseconds + 1000000000;
        } else {
            latency_seconds = received_seconds - generated_seconds;
            latency_nanoseconds = received_nanoseconds - generated_nanoseconds;
        }

        latency = latency_seconds + latency_nanoseconds / 1000000000.f;
        DEBUG_PRINTF("Secs,Nsecs %ld.%ld\n",latency_seconds, latency_nanoseconds);
        DEBUG_PRINTF("Lat.(Secs) %f\n", latency);

        printf("[%ld.%ld] Recibido mensaje topic: %s - mensaje: %s - Generó: %ld.%ld"
        "- Recibido: %ld.%ld - Latencia: %f\n",
        time.tv_sec, time.tv_nsec, client_topic, publish.data, 
        generated_seconds, generated_nanoseconds, received_seconds,
        received_nanoseconds, latency); 
    }
}

void start_data_transfer(){

    if (client_type == PUBLISHER){
        transfer_publisher();
    } else {
        transfer_subscriber();
    }
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
    DEBUG_PRINTF ("%s: Connected to server...\n",proc_name );
};

void talk_to_server(char *topic, char *ip, int port){

    DEBUG_PRINTF("TALK\n");

    if (strcmp(proc_name,"Publisher") == 0){
        DEBUG_PRINTF("PUBLISHER\n");
        client_type = PUBLISHER;
    } else {
        DEBUG_PRINTF("SUBSCRIBER\n");
        client_type = SUBSCRIBER;
    }

    strcpy(client_topic, topic);

    struct timespec time;
    long time_seconds;
    long time_nanoseconds;

    struct response response;

    struct message message;
    strcpy(message.topic, topic);

    if (client_type == PUBLISHER){
        DEBUG_PRINTF("ACTION %d\n", REGISTER_PUBLISHER);
        message.action = REGISTER_PUBLISHER;
    } else {
        DEBUG_PRINTF("ACTION %d\n", REGISTER_SUBSCRIBER);
        message.action = REGISTER_SUBSCRIBER;    
    }

    client_socket = socket_create();

    socket_connect(client_socket);

    clock_gettime(CLOCK_REALTIME, &time);
    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;

    if (client_type == SUBSCRIBER){
        printf("[%ld.%ld] %s conectado con broker (%s:%d)\n",
        time_seconds, time_nanoseconds, proc_name, ip, port);
    } else {
        printf("[%ld.%ld] %s conectado con broker correctamente\n",
        time_seconds, time_nanoseconds, proc_name);
    }

    DEBUG_PRINTF("action: %d, topic: %s\n",message.action, message.topic);  
    if (send(client_socket, (void *)&message, sizeof(message), 0) < 0){
        warnx("send() failed. %s\n", strerror(errno));
        exit(1);
    }

    if (recv(client_socket, (void *)&response, sizeof(response), 0) < 0){
        warnx("recv() failed. %s\n", strerror(errno));
        exit(1);
    }

    client_id = response.id;

    clock_gettime(CLOCK_REALTIME, &time);
    time_seconds = time.tv_sec;
    time_nanoseconds = time.tv_nsec;
    if (response.response_status == OK){
        printf("[%ld.%ld] Registrado correctamente con ID: %d para topic %s\n",
        time_seconds, time_nanoseconds, response.id, topic);    
    } else {
        printf("[%ld.%ld] Error al hacer el registro: error=%d\n",
        time_seconds, time_nanoseconds, response.response_status);
        exit(1);       
    }

    DEBUG_PRINTF("Message sent\n"); 

    start_data_transfer();

}