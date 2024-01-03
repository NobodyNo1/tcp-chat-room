#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <sys/socket.h>
#include "../common/common.h"
#include "../common/communication.h"
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

int server_socket;

int connect_client_to_server(void) {
    // create socket
    // bind socket
    server_socket = create_server_socket();
    if(server_socket == -1) {
        perror("Unable to create socket");
        return -1;
    }
    int connection = connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if(connection == -1){
        perror("Unable to connect to server");
        close(server_socket);
        return -1;
    }

    return 0;
}

int send_client_username(StringView name) {
    ClientSignInData* data = (ClientSignInData*) malloc(sizeof(ClientSignInData));
    data->name = name;
    
    int result = send_to_server_sign_in_data(server_socket, data);
    free(data);
    return result;
}

int send_client_message(StringView message) {
    ClientMessageData* data = (ClientMessageData*) malloc(sizeof(ClientMessageData));
    data->message = message;
    
    int result = send_to_server_chat_message(server_socket, data);
    free(data);
    return result;
}


#endif /* CLIENT_H */
