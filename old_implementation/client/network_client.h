#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <string.h>
#include <unistd.h>

#include "../common/common.h"
#include <pthread.h>

int network_socket;

int connect_to_server() {
    // create a socket
    int network_socket = init_server_socket();
   
    int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));

    if(connection_status != 0) {
        printf("Error connecting to server\n");
        return -1;
    }
    return network_socket;
}

int init_client(void) {
    network_socket = connect_to_server();
    if(network_socket == -1) {
        return -1;
    }
    return 0;
}

int close_client(void) {
    return close(network_socket);
}

int send_message(char* text) {
    send(network_socket, text, sizeof(char)*strlen(text), 0);
    // close(network_socket);
    return 0;
}

typedef struct {
    void (*on_new_message)(char*);
    void (*on_disconnect)(void);
} EventListener;

void* new_message_handler(void* listener) {
    EventListener eventListener = *((EventListener*)listener);
    int n;
    while (true)
    {
        char result[MAX_MESSAGE_LENGTH];
        n = read(
            network_socket,
            &result,
            sizeof(char)*MAX_MESSAGE_LENGTH
        );
        if(n < 0) {
            perror("Error reading from socket");
        } else if (n == 0){
            //disconnect
            eventListener.on_disconnect();
            close(network_socket);
            pthread_exit(NULL);
        } else {
            eventListener.on_new_message(result);
        }
    }
}


#endif // NETWORK_CLIENT_H