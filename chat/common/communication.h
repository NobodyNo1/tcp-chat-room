#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdbool.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include "string_view.h"

#define DISCONNECT_FLAG         -2

#define CLIENT_SIGN_IN_DATA_TYPE 1
#define CLIENT_MESSAGE_DATA_TYPE 2
#define SERVER_BROADCAST_MESSAGE_DATA_TYPE 3

typedef struct {
    char type;          //  type of data
    int size;           //  4 byte define size of data
} MessageDescriptor;

typedef struct {
    StringView name;
} ClientSignInData;

typedef struct {
    StringView message;
} ClientMessageData;

typedef struct {
    int from_size;  // If size == 0, it means server message (e.g. "'Username' is connected to chat")
    char* from;
    int message_size;
    char* message;
} ChatMessageData;

typedef struct {
    StringView message;
} ServerBroadcastMessage;

// START Contract transactions

char* create_descriptor(char type, int size) {
    MessageDescriptor message_descriptor = {type, size};
    char* buffer = (char*) malloc(sizeof(MessageDescriptor));
    int offset = 0;
    // Serialize each field
    memcpy(buffer + offset, &(message_descriptor.type), sizeof(message_descriptor.type));
    offset += sizeof(message_descriptor.type);
    memcpy(buffer + offset, &(message_descriptor.size), sizeof(message_descriptor.size));

    return buffer;
}

int send_contract_size(int socket_id, int type, int size) {
    char* message_descriptor = create_descriptor(type, size);
    int result = send(socket_id, message_descriptor, sizeof(MessageDescriptor), 0);
    free(message_descriptor);
    return result;
}

// todo: on disconnect handler?
int recieve_contract_size(int socket_id, MessageDescriptor* descriptor_result) {
    char* message_descriptor = (char*) malloc(sizeof(MessageDescriptor));
    int result = recv(socket_id, message_descriptor, sizeof(MessageDescriptor), 0);
    if (result == -1) {
        free(message_descriptor);
        // todo: handle result of status
        printf("Failed to handle\n");
        //TODO: handle correctly
        return -1;
    } else if(result == 0) {
        free(message_descriptor);
        printf("Client Disconnected\n");
        // todo: close socket
        return DISCONNECT_FLAG;
    }
    int offset = 0;

    // Deserialize each field
    memcpy(&(descriptor_result->type), message_descriptor + offset, sizeof(descriptor_result->type));
    offset += sizeof(descriptor_result->type);

    memcpy(&(descriptor_result->size), message_descriptor + offset, sizeof(descriptor_result->size));

    return 0;
}

// END Contract transactions

// START user sign in

int send_to_server_sign_in_data(int server_socket, ClientSignInData* data) {
    // send data size
    if(send_contract_size(server_socket, CLIENT_SIGN_IN_DATA_TYPE, data->name.size) == -1){
        return -1;
    }

    // send data itself
    if(send(server_socket, data->name.text, data->name.size, 0) == -1)
    {
        perror("Unable to send message to server\n");
        return -1;
    }
    // success
    return 0;
}

int read_from_client_sign_in_data(int socket_id, StringView* client_name) {
    MessageDescriptor message_descriptor;
    int contract_result = recieve_contract_size(socket_id, &message_descriptor);
    if(contract_result != 0) {
        return contract_result;
    }
    if(message_descriptor.size == 0) {
        return -1;
    }

    // TODO: investigate it is possible to make this logic generic?
    char* name_result = (char*) malloc(message_descriptor.size);
    int result = recv(socket_id, name_result, message_descriptor.size, 0);
    
    client_name->size = message_descriptor.size;

    if (result == -1) {
        free(name_result);
        // todo: handle result
        perror("Error Reading value\n");
        return -1;
    } else if(result == 0){
        free(name_result);
        // TODO: close socket
        printf("Client Disconnected\n");
        return DISCONNECT_FLAG;
    }
    client_name->text = name_result;

    return 0;
}

// END user sign in

// START user message handling
int send_to_server_chat_message(int server_socket, ClientMessageData* data) {
    // send data size
    if(send_contract_size(server_socket, CLIENT_MESSAGE_DATA_TYPE, data->message.size) == -1){
        return -1;
    }
    // send data itself
    if(send(server_socket, data->message.text, data->message.size, 0) == -1)
    {
        perror("Unable to send message to server\n");
        return -1;
    }
    return 0;
}

int read_from_client_message_data(int client_socket, StringView* client_message) {
    MessageDescriptor message_descriptor;
    int contract_result = recieve_contract_size(client_socket, &message_descriptor);
    if(contract_result != 0) {
        return contract_result;
    }
    if(message_descriptor.size == 0) {
        return -1;
    }

    // TODO: investigate it is possible to make this logic generic?
    char* message_result = (char*) malloc(message_descriptor.size);
    int result = recv(client_socket, message_result, message_descriptor.size, 0);

    client_message->size = message_descriptor.size;

    if (result == -1) {
        free(message_result);
        // todo: handle result
        perror("Error Reading value\n");
        return -1;
    } else if(result == 0){
        free(message_result);
        printf("Client Disconnected\n");
        return DISCONNECT_FLAG;
    }
    //message_result
    client_message->text = message_result;

    return 0;
}

// END user message handling

// TODO: what if broadcast happens while client sending message? Mutex?
int client_read_broadcast_message(int server_socket, StringView* server_message) {
    MessageDescriptor message_descriptor;
    int contract_result = recieve_contract_size(server_socket, &message_descriptor);
    if(contract_result != 0) {
        return contract_result;
    }
    if(message_descriptor.type != SERVER_BROADCAST_MESSAGE_DATA_TYPE){
        return -1;
    }
    if(message_descriptor.size == 0) {
        return -1;
    }

    // TODO: investigate it is possible to make this logic generic?
    char* message_result = (char*) malloc(message_descriptor.size+1);
    int result = recv(server_socket, message_result, message_descriptor.size, 0);
    message_result[message_descriptor.size] = '\0';

    server_message->size = message_descriptor.size;

    if (result == -1) {
        free(message_result);
        // todo: handle result
        perror("Error Reading value\n");
        return -1;
    } else if(result == 0){
        free(message_result);
        printf("Server is Disconnected\n");
        return DISCONNECT_FLAG;
    }
    //message_result
    server_message->text = message_result;

    return 0;
}

// Currently sends plain text
int server_broadcast_message(int* sock_clients, int client_size, char* message) {
    if(client_size == 0) {
        printf("No clients to broadcast\n");
        return 0;
    }
    ServerBroadcastMessage* data = (ServerBroadcastMessage*) malloc(sizeof(ServerBroadcastMessage));
    data->message = create_string_view(message);
    int count = client_size;
    for (int i = 0; i < client_size; i++)
    {
        if(send_contract_size(sock_clients[i], SERVER_BROADCAST_MESSAGE_DATA_TYPE, data->message.size) == -1){
            perror("Unable to send contract size\n");
            count--;
            continue;
        }
        // send data itself
        if(send(sock_clients[i], data->message.text, data->message.size, 0) == -1)
        {
            count--;
            perror("Unable to send message to client\n");
            continue;
        }
    }
    
    return count;
}

#endif