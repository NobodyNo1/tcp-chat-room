#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "network_client.h"
#include <pthread.h>

#define MAX_INPUTS 10
#define MAX_INPUT_LENGTH MAX_MESSAGE_LENGTH

char currentInput[MAX_INPUT_LENGTH];

// Structure to store previous inputs
typedef struct {
    char inputs[MAX_INPUTS][MAX_INPUT_LENGTH];
    int count;
} InputHistory;


// Function to cleanup ncurses
void cleanupNcurses() {
    endwin();   // Cleanup ncurses
}

// Function to initialize ncurses
void initNcurses() {
    initscr();              // Initialize ncurses
    raw();                  // Disable line buffering
    keypad(stdscr, TRUE);   // Enable special keys
    noecho();               // Do not display input
}

InputHistory history;
pthread_mutex_t history_locker;
int connection = 0;
// TODO: scroll?
// Function to draw the TUI
void drawUI() {
    pthread_mutex_lock(&history_locker);
    clear();    // Clear the screen
    if(!connection){
        printw("Disconnected!");
        refresh();  // Refresh the screen
        pthread_mutex_unlock(&history_locker);
        return;
    }
    // Draw previous inputs
    mvprintw(1, 1, "Chat log:");
    // TODO: limit size
    
    for (int i = 0; i < history.count; i++) {
        mvprintw(2 + i, 1, "%s", history.inputs[i]);
    }

    // Draw input field
    mvprintw(LINES - 2, 1, "Current Input: %s", currentInput);
    mvprintw(LINES - 1, 1, "Press Enter to submit, ESC to quit");

    refresh();  // Refresh the screen
    pthread_mutex_unlock(&history_locker);
}

bool is_alpha_num(char ch) {
    if('a' <= ch && ch <= 'z') return true;
    if('A' <= ch && ch <= 'Z') return true;
    if('0' <= ch && ch <= '9') return true;
    return false;
}

bool is_allow_symb(char ch) {
    if(' ' == ch) return true;
    if(',' == ch) return true;
    if('.' == ch) return true;
    if('?' == ch) return true;
    if('!' == ch) return true;
    if(':' == ch) return true;
    if(';' == ch) return true;
    return false;
}

void on_new_message(char* text) {
    pthread_mutex_lock(&history_locker);
    if (history.count < MAX_INPUTS) {
        strcpy(history.inputs[history.count], text);
        history.count++;
    } else {
        // If the history is full, shift elements
        for (int i = 0; i < MAX_INPUTS - 1; i++) {
            strcpy(history.inputs[i], history.inputs[i + 1]);
        }
        strcpy(history.inputs[MAX_INPUTS - 1], text);
    }
    pthread_mutex_unlock(&history_locker);
    drawUI();
}

void on_disconnect() {
    connection = 0;
    drawUI();
}

pthread_t listener_thread;
void init_listener(){
    // EventListener listener = {
    //     .on_new_message = &on_new_message,
    //     .on_disconnect = &on_disconnect
    // };
    EventListener* listener = (EventListener*) malloc(sizeof(EventListener));
    listener->on_new_message = &on_new_message;
    listener->on_disconnect = &on_disconnect;

    pthread_create(
        &listener_thread,
        NULL,
        new_message_handler, 
        listener
    );
    pthread_detach(listener_thread);
}


int main() {
    if(init_client() == -1) return 0;

    connection = 1;
    init_listener();

    history.count = 0;

    currentInput[0] = '\0';
    char send_queue[MAX_INPUT_LENGTH];

    // Initialize ncurses
    initNcurses();

    int ch;
    while (1) {
        if(send_queue[0] != '\0'){
            send_message(send_queue);
            send_queue[0] = '\0';
        } else {
            drawUI();
        }
        // Draw the UI

        // Get user input
        ch = getch();

        // Handle user input
        if (ch == '\n') {  // Enter key pressed
            if (strlen(currentInput) > 0) {
                // Add current input to history
                // TODO: might lag
                strcpy(send_queue, currentInput);
                // Clear current input
                for(int p = 0; p <MAX_INPUT_LENGTH; p++){
                    currentInput[p] = ' ';
                }
                currentInput[0] = '\0';
            }
        } else if (ch == 127) {  // Backspace key pressed
            int len = strlen(currentInput);
            if (len > 0) {
                currentInput[len - 1] = '\0';
            }
        } else if (ch == 27) {  // 'q' key pressed
            break;  // Exit the loop and quit the program
        } else if(is_alpha_num(ch) || is_allow_symb(ch)) {
            
            // Append other characters to current input
            strncat(currentInput, (char*)&ch, 1);
        }
    }
    close_client();
    // Cleanup ncurses
    cleanupNcurses();

    return 0;
}
