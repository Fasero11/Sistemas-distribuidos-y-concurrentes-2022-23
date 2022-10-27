// Gonzalo Vega Pérez - 2022

#include "proxy.h"

// Establece el nombre del proceso (para los logs y trazas)
void set_name (char name[2]){
    printf("change name 2\n");
};
    
// Establecer ip y puerto
void set_ip_port (char* ip, unsigned int port);

// Obtiene el valor del reloj de lamport.
// Utilízalo cada vez que necesites consultar el tiempo.
// Esta función NO puede realizar ningún tiempo de comunicación (sockets)
int get_clock_lamport();

// Notifica que está listo para realizar el apagado (READY_TO_SHUTDOWN)
void notify_ready_shutdown();

// Notifica que va a realizar el shutdown correctamente (SHUTDOWN_ACK)
void notify_shutdown_ack();

// Envía un mensaje e imprime una traza
void send(struct message msg){
    printf("%s, %d, SEND, %d",msg.origin, msg.clock_lamport, msg.action);
    
};

// Recive un mensaje e imprime una traza
void recieve(char *prc, struct message msg){
    printf("%s, %d, RECV %s, %d",msg.origin, msg.clock_lamport, prc, msg.action);
};