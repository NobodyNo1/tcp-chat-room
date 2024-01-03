#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "../common/common.h"

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_CLIENTS 10
Client clients[MAX_CLIENTS];

pthread_mutex_t client_mutex;
int client_size = 0;

void add_client(int client_socket) {
    clients[client_size++].socket = client_socket;
}

void *handle_client(void *arg) {
    int client_socket = *((int*)arg);
    char buffer[MAX_MESSAGE_LENGTH];
    int n;

    while (1)
    {
        // empty buffer
        for (int i = 0; i < MAX_MESSAGE_LENGTH; i++)
        {
            buffer[i] = ' ';
        }
        // Read and write data with the client as needed
        // Example:
        n = read(client_socket, &buffer, sizeof(buffer));
        if (n < 0) {
            perror("Error reading from socket");
        } else if (n == 0) {
            // Connection closed by client
            printf("Client disconnected\n");
            remove_client(client_socket);
            close(client_socket);
            pthread_exit(NULL);
        } else {
            // Process data and/or respond to the client
            buffer[MAX_MESSAGE_LENGTH - 1] = '\0';
            printf("Received message from client: %s\n", buffer);
            //broadcast to all
            broadcast_message(buffer, client_socket);
            // send(client_socket, &buffer, MAX_MESSAGE_LENGTH*sizeof(char), 0); 
        }
    }
}

void remove_client(int id) {
    //todo: mutex
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {   
        int client_socket = clients[i].socket;
        if(id == client_socket){
            clients[i].socket = 0;
            clients[i].name = "\0";
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void broadcast_message(char* buffer, int from) {
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {   
        int client_socket = clients[i].socket;
        if(client_socket != 0)
            write(client_socket, buffer, MAX_MESSAGE_LENGTH*sizeof(char));        
    }
    pthread_mutex_unlock(&client_mutex);
}


void close_connections() {
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        // disconnect clients
        int client_socket = clients[i].socket;
        if (client_socket != 0)
            close(client_socket);
    }
}

#endif /* CLIENT_HANDLER_H */
