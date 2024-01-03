#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include <client.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define ESC_KEY                 27

#define MAX_USER_INPUT_SIZE     20

#define SIGN_IN_STATE   1
#define CHAT_STATE      2

#define HISTORY_MAX     10
StringView history[HISTORY_MAX];
int history_size = 0;

int state = SIGN_IN_STATE;
int row,col;

char user_input[MAX_USER_INPUT_SIZE] = "\0";
int user_input_cursor = 0;

bool server_alive = false;
bool client_alive = true;

pthread_mutex_t history_processing;
pthread_mutex_t input_processing;

void add_input_value(char ch) {
    pthread_mutex_lock(&input_processing);
    if(user_input_cursor >= MAX_USER_INPUT_SIZE - 1) {
        pthread_mutex_unlock(&input_processing);
        return;
    }
    user_input[user_input_cursor++] = ch;
    user_input[user_input_cursor] = '\0';
    pthread_mutex_unlock(&input_processing);
}

void remove_input_value() {
    pthread_mutex_lock(&input_processing);
    if(user_input_cursor <= 0) {
        pthread_mutex_unlock(&input_processing);
        return;
    }
    user_input_cursor--;
    user_input[user_input_cursor] = '\0';
    pthread_mutex_unlock(&input_processing);
}

void clear_input_value() {
    pthread_mutex_lock(&input_processing);
    if(user_input_cursor == 0) {
        pthread_mutex_unlock(&input_processing);
        return;
    }
    user_input_cursor = 0;
    user_input[0] = '\0';
    pthread_mutex_unlock(&input_processing);
}

// TODO: probably better to create map with symbols as keys and value as allowed or not
bool is_allow_symb(char ch) {
    return 32 <= ch && ch <= 126;
}

bool is_alpha_numberic(int ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ('0' <= ch && ch <= '9'); 
}

bool is_char_allowed(int ch, bool allow_symb) {
    if(is_allow_symb(ch) && allow_symb) return true; 
    return is_alpha_numberic(ch);
}

bool connect_to_server();
bool send_username();
bool send_message();

void set_chat_state();

void start_input_thread();
void draw();

bool new_event = false;

void start() {
    if(!connect_to_server()){
        return;
    }
    server_alive = true;
    initscr();
    raw();
    noecho();
    start_input_thread();
    new_event = true;
    while(server_alive && client_alive) {
        // dynamically adapt TUI size
        getmaxyx(stdscr,row,col);		/* get the number of rows and columns */
        if(!new_event) continue;
        new_event = false;
        draw();
    }
    server_alive = false;
    close(server_socket);
    endwin();
}

void* handle_input(void* arg) {
    int input_text;
    while(server_alive && client_alive){
        input_text = getch();

        if(input_text == ESC_KEY) {
            client_alive = false;
            pthread_exit(NULL);
            continue;
        }
        if(is_char_allowed(input_text, state == CHAT_STATE)){
            add_input_value(input_text);
            new_event = true;
            continue;
        }
        if(input_text == KEY_BACKSPACE || input_text == 8 || input_text == 127) {
            remove_input_value();
            new_event = true;
            continue;
        }
        if(input_text == '\n') {
            if(user_input_cursor == 0) continue;
            if(state == SIGN_IN_STATE) {
                //TODO: async?
                if(send_username()){
                    clear_input_value();
                    set_chat_state();
                }
                continue;
            }
            if(state == CHAT_STATE){
                //TODO: async?
                if(send_message()){
                    clear_input_value();
                }
                continue;
            }
            continue;
        }
    }
    pthread_exit(NULL);
}

void start_input_thread() {
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, handle_input, NULL);
    pthread_detach(input_thread);
}

void draw_sign_in_state();
void draw_footer();

void draw() {
    clear();
    draw_footer();
    switch (state)
    {
    case SIGN_IN_STATE:
        draw_sign_in_state();
        break;
    
    case CHAT_STATE:
        {
            char* mesg = "Chat history:";
            mvprintw(1, 0, "%s", mesg);
            pthread_mutex_lock(&history_processing);
            for(int i = 0; i < history_size; i++){
                mvprintw(i+2, 0, "%s", history[i].text);
            }
            pthread_mutex_unlock(&history_processing);
            // TODO: finish
            // int y, x;               // to store where you are
            // getyx(stdscr, y, x);    // save current pos
            // move(row-2, 0);         // move to input line
            // clrtoeol();             // clear the line
            // move(y, x);             // move back to where you were
            mvprintw(row-2, 0,"Message: %s", user_input);
            break;
        }
    }
    refresh();
}

void draw_sign_in_state() {
    // TODO: what if there not much space for the text?
    char* mesg="Welcome to the server!";
    mvprintw(1, 0, "%s", mesg);
    
    // In works because last printw is at input position
    int y, x;               // to store where you are
    getyx(stdscr, y, x);    // save current pos
    move(row-2, 0);         // move to input line
    clrtoeol();             // clear the line
    move(y, x);             // move back to where you were
    mvprintw(row-2, 0,"Input your username: %s", user_input);
    // TODO: Input field 
}

void draw_footer() {
    char* mesg= "Press Enter to input, and ESC to exit";
    char* error_mesg= "";
    mvprintw(row-1, 0, "%s  %s", mesg, error_mesg);
}

bool connect_to_server() {
    return connect_client_to_server() == 0;
}

// TODO: more proper error messaging
bool send_username() {
    StringView username = create_string_view(
        user_input
    );

    int result =  send_client_username(username);
    if(result == DISCONNECT_FLAG){
        server_alive = false;
    }
    return result == 0;
}

bool send_message() {
    StringView message = create_string_view(
        user_input
    );

    int result =  send_client_message(message);
    if(result == DISCONNECT_FLAG){
        server_alive = false;
    }
    return result == 0;
}

void update_history(StringView* server_message){
    if(history_size < HISTORY_MAX){
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
    while(client_alive && server_alive) {
        server_message = (StringView){0, NULL};
        int result = client_read_broadcast_message(server_socket, &server_message);
        if(result == DISCONNECT_FLAG) {
            break;
        }
        if(result != 0) continue;
        pthread_mutex_lock(&history_processing);
        update_history(&server_message);
        pthread_mutex_unlock(&history_processing);
        new_event = true;
    }
    server_alive = false;
    pthread_exit(NULL);
}

void set_chat_state() {
    history_size = 0;
    pthread_t message_thread;
    pthread_create(&message_thread, NULL, chat_message_thread, NULL);
    pthread_detach(message_thread);
    state = CHAT_STATE;
}

#endif /* TUI_H */


 