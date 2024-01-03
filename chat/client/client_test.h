#ifndef CLIENT_TEST_H
#define CLIENT_TEST_H

#include "client.h"
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define HISTORY_MAX 10

StringView history[HISTORY_MAX];
int history_size = 0;
pthread_mutex_t history_processing;
int server_alive = 0;


bool send_username(char* input) {
    StringView username = create_string_view(
        input
    );

    int result =  send_client_username(username);
    if(result == DISCONNECT_FLAG){
        printf("Disconnected!\n");
        return false;
    }
    return result == 0;
}

bool send_message(char* input) {
    StringView message = create_string_view(
        input
    );

    int result = send_client_message(message);
    if(result == DISCONNECT_FLAG){
        printf("Disconnected!\n");
        return false;
    }
    return result == 0;
}

bool connect_to_server() {
    return connect_client_to_server() == 0;
}

void update_history(StringView* server_message){
    if(history_size + 1 < HISTORY_MAX ){
        history[history_size++] = copy_string_view(server_message);
    } else{
        for(int i = 0; i<HISTORY_MAX-1; i++){
            history[i] = history[i+1];
        }
        history[HISTORY_MAX-1] = copy_string_view(server_message);
    }
}

void* chat_message_thread(void* arg) {
    StringView server_message;
    while(true) {
        server_message = (StringView){0, NULL};
        int result = client_read_broadcast_message(server_socket, &server_message);
        if(result == DISCONNECT_FLAG) {
            break;
        }
        if(result != 0) continue;
        pthread_mutex_lock(&history_processing);
        printf("recieved: %s \n", server_message.text);
        update_history(&server_message);
        pthread_mutex_unlock(&history_processing);
    }
    server_alive = 0;
    pthread_exit(NULL);
}

void start() {
    if(!connect_to_server()){
        return;
    }
    send_username("Bekarys");
    sleep(1);

    history_size = 0;
    pthread_t message_thread;
    pthread_create(&message_thread, NULL, chat_message_thread, NULL);
    pthread_detach(message_thread);

    send_message("Hey hey");
    sleep(1);
    send_message("Hey hey");
    sleep(2);
}

#endif /* CLIENT_TEST_H */
