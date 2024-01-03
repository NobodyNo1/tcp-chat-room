
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/communication.h"
#include <stdbool.h>
#include "../common/common.h"

typedef struct {
    int client_socket;
    StringView name;
} ClientInfo;

int connected_clients = 0;
ClientInfo info[MAX_CLIENTS];

bool server_is_full() {
    return connected_clients >= MAX_CLIENTS;
}

void init_client_handler(){
    for(int i = 0; i < MAX_CLIENTS; i++){
        info[i].client_socket = -1;
        info[i].name = (StringView){ 0, NULL };
    }
}

void* client_handler(void* arg);

void handle_client(int client_socket) {
    info[connected_clients++].client_socket = client_socket;
    pthread_t thr;
    if(pthread_create(&thr, NULL, client_handler, (void*) &client_socket) != 0) {
        perror("Unable to create for client hanlder\n");
    }
    
    pthread_detach(thr);
}

void broadcast_message(char* text) {
    int clients[MAX_CLIENTS];
    for(int i = 0, j = 0; i < MAX_CLIENTS; i++){
        if(info[i].client_socket != 0)
            clients[j++] = info[i].client_socket;
    }
    int broadcasted_to = server_broadcast_message(clients, connected_clients, text);
    // printf("Send to %d clients\n", broadcasted_to);
}

void* client_handler(void* arg) {
    int client_socket = *((int*)arg);
    printf("Handling client %d\n", client_socket);  
    int result = -1;
    StringView client_name = {0, NULL};
    while(result != 0){
        result = read_from_client_sign_in_data(client_socket, &client_name); 
        if(result == DISCONNECT_FLAG) goto end_connection;
    }
    printf("Client connected: %s, %d\n", client_name.text, client_name.size);
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(info[i].client_socket == client_socket){
            info[i].name.size = client_name.size;
            info[i].name.text = client_name.text;
            break;
        }
    }
    // TODO: broacast about user connection
    char *s = (char*)malloc(40 * sizeof(char)); // 20 username max + 15 text
    sprintf(s, "<%s> connected",client_name.text);
    broadcast_message(s);

    StringView client_message = {0, ""};
    while(true) {
        result = read_from_client_message_data(client_socket, &client_message);
        if(result == DISCONNECT_FLAG) break;
        
        if(result == -1) continue;
        //TODO: broadcast!
        printf("Got Message: %s > %s\n", client_name.text, client_message.text);
        
        char *s = (char*)malloc(60 * sizeof(char)); // 20 username max + 20 message max + 10 format
        sprintf(s, "<%s>: %s", client_name.text, client_message.text);
        broadcast_message(s);
    }
end_connection:
    // TODO: broacast about user disconnection
    // todo: thread safety
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(info[i].client_socket == client_socket){
            info[i] = (ClientInfo){ 0, {0, NULL} };
        }
    }
    connected_clients--;
    s = (char*)malloc(40 * sizeof(char)); // 20 username max + 15 text
    sprintf(s, "<%s> disconnected",client_name.text);
    broadcast_message(s);

    close(client_socket);
    pthread_exit(NULL);
    return NULL;
}

