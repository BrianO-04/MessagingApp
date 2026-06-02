#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "cursesUI.h"
#include "messagelog.h"
#include "aes.h"

THRDFUNC init_ui(void* arg){

    void **ui_args = (void**)arg;

    int* ui_initialized = (int*)ui_args[0];
    int* new_message = (int*)ui_args[1];
    struct log* message_log = (struct log*)ui_args[2];
    mtx_t *log_lock = (mtx_t*)ui_args[3];
    int* client_fd = (int*)ui_args[4];

    *ui_initialized = 1;

    // TINY AES SETUP
    struct AES_ctx aes_ctx;
    uint8_t iv[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&aes_ctx, aes_key, iv);

    // Set up window
    initscr();
    cbreak();
    noecho();
    WINDOW* win = newwin(32, MESSAGE_LEN/2, 0, 0);
    keypad(win, true);
    meta(win, TRUE);

    // Print initial border box
    box(win, 0, 0);
    wrefresh(win);

    void **input_args = malloc(sizeof(void*)*7);
    char msg[MESSAGE_LEN] = { 0 };
    input_args[0] = msg;
    input_args[1] = win;
    input_args[2] = &aes_ctx;
    input_args[3] = client_fd;
    input_args[4] = message_log;
    input_args[5] = log_lock;
    input_args[6] = new_message;

    thrd_t input_t;
    thrd_create(&input_t, input_thread, input_args);

    while(1){
        if(*new_message == 1){
            werase(win);
            mtx_lock(log_lock);

            int line = 1;

            struct message* curr = message_log->head;
            while(curr != NULL){

                // Decrypt current message
                AES_init_ctx_iv(&aes_ctx, aes_key, curr->iv);
                char decrypted_msg[MESSAGE_LEN];
                memcpy(decrypted_msg, curr->msg, MESSAGE_LEN);
                AES_CBC_decrypt_buffer(&aes_ctx, (uint8_t*)decrypted_msg, MESSAGE_LEN);

                mvwprintw(win, line++, 1, "%s: %s", curr->usr, decrypted_msg);
                curr = curr->next;
            }

            *new_message = 0;
            
            mtx_unlock(log_lock);
        }else{
            wmove(win, 30, 1);
            wclrtoeol(win);
        }
        mvwprintw(win, 30, 1, "%s", msg);
        box(win, 0, 0);
        wrefresh(win);
    }

    free(input_args);

    thrd_exit(THRDEXIT);
    return THRDEXIT;
}

THRDFUNC input_thread(void* arg){
    void **inp_args = (void**)arg;

    char *msg = (char*)inp_args[0];
    WINDOW* win = (WINDOW*)inp_args[1];
    struct AES_ctx* aes_ctx = (struct AES_ctx*)inp_args[2];
    int client_fd = *(int*)inp_args[3];
    struct log* message_log = (struct log*)inp_args[4];
    mtx_t *log_lock = (mtx_t*)inp_args[5];
    int* new_message = (int*)inp_args[6];

    int ind = 0;
    while(1){
        while(ind < MESSAGE_LEN-1){
            int curr = wgetch(win);
            if(curr == '\n') break;
            else if(curr == '\b' || curr == 127 || curr == KEY_BACKSPACE){
                if(ind > 0){
                    // Delete last char
                    msg[--ind] = '\0';
                }
                continue;
            }
            msg[ind++] = curr;
            msg[ind] = '\0';
        }
        if(msg[0] == '\0'){
            continue;
        }

        //Send message
        cmd_types msg_cmd = MESSAGE;
        send(client_fd, &msg_cmd, sizeof(cmd_types), 0);

        send(client_fd, aes_ctx->Iv, AES_BLOCKLEN, 0);

        uint8_t iv[AES_BLOCKLEN] = { 0 };
        memcpy(iv, aes_ctx->Iv, AES_BLOCKLEN);

        //Encrypt message
        char encrypted[MESSAGE_LEN];
        memcpy(encrypted, msg, MESSAGE_LEN);
        AES_CBC_encrypt_buffer(aes_ctx, (uint8_t*)encrypted, MESSAGE_LEN);
   
        send(client_fd, encrypted, MESSAGE_LEN, 0);

        mtx_lock(log_lock);
        add_log(message_log, encrypted, "SELF", iv);
        *new_message = 1;
        mtx_unlock(log_lock);

        memset(msg, '\0', MESSAGE_LEN);
        ind = 0;
    }

    thrd_exit(THRDEXIT);
    return THRDEXIT;
}

int test(int argc, char *argv[]){
    // 1. Initialize the screen
    initscr();            // Start curses mode
    cbreak();             // Line buffering disabled, Pass on evertyhing to me
    noecho();             // Don't echo() while we do getch
    keypad(stdscr, TRUE); // Enable special keys like F1, F2, arrow keys

    // 2. Create a simple window (height, width, start_y, start_x)
    int height = 32, width = MESSAGE_LEN/2, start_y = 0, start_x = 0;
    WINDOW *win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE); // Enable special keys like F1, F2, arrow keys
    
    // 3. Draw a border and text inside the window
    box(win, 0, 0);       // Draw a default border

    // 4. Refresh to show changes
    refresh();            // Refresh standard screen
    wrefresh(win);        // Refresh our specific window


    int currLine = 1;
    while(currLine < 30){
        char msg[MESSAGE_LEN] = { 0 };
        int curr = '\0';
        int ind = 0;
        while(ind < MESSAGE_LEN-1){
            curr = wgetch(win);

            if(curr == '\n') break;
            else if(curr == '\b' || curr == 127 || curr == KEY_BACKSPACE){
                if(ind > 0){
                    // Delete last char
                    msg[--ind] = '\0';
                    // Clear line
                    wmove(win, 30, 1);
                    wclrtoeol(win);
                    // Re-print message
                    mvwprintw(win, 30, 1, "%s", msg);
                    // Re-draw box
                    box(win, 0, 0);
                    wrefresh(win);
                }
                continue;
            }
            msg[ind++] = curr;
            msg[ind] = '\0';

            mvwprintw(win, 30, 1, "%s", msg);
            wrefresh(win);
        }
        if(strcmp(msg, "\0") == 0){
            continue;
        }
        // Clear input box
        wmove(win, 30, 1);
        wclrtoeol(win);
        box(win, 0, 0);

        // Print new message
        mvwprintw(win, currLine++, 1, "%s", msg);
        wmove(win, 30, 1);
        wrefresh(win);
    }

    // 6. Clean up
    endwin();             // End curses mode
    return 0;
}