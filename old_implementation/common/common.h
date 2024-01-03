#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define MAX_MESSAGE_LENGTH 50
#define PORT        9002

typedef struct
{
    int socket;
    char* name;
} Client;

typedef struct
{
    int type;
} ServerMessage;

typedef struct
{
    char* from_name;
    char* text;
} ChatMessage;

ChatMessage* create_message(int from, char* text) {
    ChatMessage* message = malloc(sizeof(ChatMessage));
    message->from_name = from;
    message->text = text;
    return message;
}

ChatMessage* create_empty_message(int from) {
    ChatMessage* message = malloc(sizeof(ChatMessage));
    message->from_name = from;
    message->text = (char*) malloc(MAX_MESSAGE_LENGTH*sizeof(char));
    for (int i = 0; i < MAX_MESSAGE_LENGTH; i++)
    {
        message->text[i] = ' ';
    }
    message->text[0] = '\0';
    
    return message;
}
struct sockaddr_in server_address = {
    .sin_family = AF_INET,
    .sin_port = htons(PORT),
    .sin_addr.s_addr = INADDR_ANY
};

int init_server_socket() {
    return socket(AF_INET, SOCK_STREAM, 0); // 0 - TCP
}

#endif /* COMMON_H */
