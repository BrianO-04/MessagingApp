#if defined(_WIN32)
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "cursesUI.h"
#include "messagelog.h"

#include "client.h"

#define IN_LINE 31

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Expected usage: ./MessagingApp {name} {IP}\n");
        return EXIT_FAILURE;
    }
    // Client backend setup
    int* new_message = malloc(sizeof(int));
    mtx_t* log_lock = malloc(sizeof(mtx_t));

    struct log* message_log = client_init(argv[1], argv[2], new_message, log_lock);
    if(message_log == NULL){
        printf("Failed to initialize client\n");
        return -1;
    }

    // Curses window setup
    initscr();
    cbreak();
    noecho();
    WINDOW* win = newwin(33, MESSAGE_LEN/2, 0, 0);
    keypad(win, true);
    
    wtimeout(win, 100);

    // Print initial border box
    box(win, 0, 0);
    wrefresh(win);

    char msg[MESSAGE_LEN];
    memset(msg, '\0', MESSAGE_LEN);

    int curr = '\0';
    int ind = 0;
    while(1){
        // Input logic
        curr = wgetch(win);
        if(curr != ERR){
            if(curr == '\n'){
                if(strcmp(msg, "/EXIT") == 0){
                    send_code(USR_EXIT);

                    werase(win);
                    wrefresh(win);

                    break;
                }else{
                    send_msg(msg);

                    memset(msg, '\0', MESSAGE_LEN);
                    ind = 0;
                }
            }else if(curr == '\b' || curr == 127 || curr == KEY_BACKSPACE || curr == 8){
                if(ind > 0){
                    // Delete last char
                    msg[--ind] = '\0';
                    // Clear line
                    wmove(win, IN_LINE, 1);
                    wclrtoeol(win);
                    // Re-print message
                    mvwprintw(win, IN_LINE, 1, "%s", msg);
                    // Re-draw box
                    box(win, 0, 0);
                    wrefresh(win);
                }
            }else{
                if(ind < MESSAGE_LEN-1){
                    msg[ind++] = curr;
                    msg[ind] = '\0';
                }
            }
        }

        // Message Display Logic
        mtx_lock(log_lock);
        if(*new_message == 1){
            werase(win);

            int line = 1;

            struct message* curr = message_log->head;
            while(curr != NULL){
                // Decrypt current message
                char decrypted_msg[MESSAGE_LEN];
                decrypt_msg(decrypted_msg, curr);

                mvwprintw(win, line++, 1, "%s: %s", curr->usr, decrypted_msg);
                curr = curr->next;
            }

            *new_message = 0;

        }else{
            wmove(win, IN_LINE, 1);
            wclrtoeol(win);
        }
        mtx_unlock(log_lock);
        mvwprintw(win, IN_LINE, 1, "%s", msg);
        box(win, 0, 0);
        wrefresh(win);
    }

    free(new_message);
    mtx_destroy(log_lock);
    free(log_lock);
    // Properly close the client listening thread later

    return 0;
}