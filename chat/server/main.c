
#include <stdio.h>
#include <stdlib.h>         //exit values
#include <stdbool.h>

#include <sys/socket.h>     //socket, AF_INET

#include <unistd.h>         //write, read

#include <netinet/in.h>     // INADDR_ANY, sockaddr_in, IPPROTO_TCP

#include "connection_handle.h"
#include "../common/common.h"

void start_server(void);

int main(void) {
    start_server();
    return 0;
}
int server_socket;

void clean_up() {
    printf("\nExiting server...\n");
    // TODO: stopping threads? 
    close(server_socket);
}

void signalHandler()
{
    exit(EXIT_FAILURE);
}

// TODO: Server TUI, or safe exit?
void start_server(void) {
    server_socket = create_server_socket();
    if(server_socket == -1) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }
    // Unexpected exit handling
    atexit(clean_up);
    signal(SIGINT, signalHandler);
    // bind socket
    // Expanation of casting https://stackoverflow.com/a/57431271
    if(bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        perror("Unable to bind socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // listen socket
    if(listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Unable to listen socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Listening at port: %d ...\n", PORT);
    init_client_handler();
    while(true) {
        if(server_is_full()) continue;
        int client_socket = accept(server_socket, NULL, NULL);
        if(client_socket == -1) {
            perror("Unable to accept socket");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        // Handle connection
        handle_client(client_socket);
    }
    close(server_socket);
    exit(EXIT_SUCCESS);
}


