#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include "../common/common.h"
#include "client_handler.h"

#include <pthread.h>
#include <unistd.h>


void *handle_client(void *arg);
void broadcast_message(char* buffer, int from);
void remove_client(int id);
void close_server();

int server_socket;
int is_server_active = 1;

void* input_handler() {
    printf("Input 'Q' to exit the server");
    char exit_command[2];
    while (1)
    {
        // Read input for exit
        fgets(exit_command, sizeof(exit_command), stdin);

        if (exit_command[0] == 'Q')
        {
            printf("Exiting the server...\n");
            is_server_active = 0;
            close_server();
            exit(EXIT_FAILURE);
            break;
        }
    }
}

int main(void) {
    // create server socket
    server_socket = init_server_socket(); 
    
    if(bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        perror("Error binding socket");
        return 0;
    }

    listen(server_socket, MAX_CLIENTS);
	printf("Listening at port %d ...\n", PORT);
    
    pthread_t exit_thread;

    // Create a thread to listen for "Q" input
    if (pthread_create(&exit_thread, NULL, input_handler, NULL) != 0)
    {
        perror("Error creating input handler thread");
        exit(EXIT_FAILURE);
    }


    int client_socket;
    is_server_active = 1;
    while(is_server_active) {
        if(client_size >= MAX_CLIENTS) continue;//todo:sleep? 
        printf("LOOP %d\n", client_size);
        client_socket = accept(server_socket, NULL, NULL);
        add_client(client_socket);
        printf("Connected %d\n", client_socket);
        if(client_socket < 0){
            close_server();
            perror("Error accepting connection");
            return 0;
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void*)&client_socket) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }

        // // Detach the thread to allow it to run independently
        pthread_detach(thread);

    }
    close_server();
    return 0;
}


void close_server() {
    close_connections();
    close(server_socket);
}
