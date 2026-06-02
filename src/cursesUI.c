#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "cursesUI.h"
#include "messagelog.h"
#include "aes.h"

#define IN_LINE 31

THRDFUNC init_ui(void* arg){

    void **ui_args = (void**)arg;

    int* new_message = (int*)ui_args[0];
    struct log* message_log = (struct log*)ui_args[1];
    mtx_t *log_lock = (mtx_t*)ui_args[2];
    int client_fd = *(int*)ui_args[3];

    //*ui_initialized = 1;

    // TINY AES SETUP
    // Encryption ctx
    struct AES_ctx enc_ctx;
    uint8_t iv1[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&enc_ctx, aes_key, iv1);
    // Decryption ctx
    struct AES_ctx dec_ctx;
    uint8_t iv2[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&dec_ctx, aes_key, iv2);

    // Set up window
    initscr();
    cbreak();
    noecho();
    WINDOW* win = newwin(34, MESSAGE_LEN/2, 0, 0);
    keypad(win, true);
    meta(win, TRUE);
    
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
                 //Send message
                cmd_types msg_cmd = MESSAGE;
                send(client_fd, &msg_cmd, sizeof(cmd_types), 0);

                send(client_fd, enc_ctx.Iv, AES_BLOCKLEN, 0);

                uint8_t iv[AES_BLOCKLEN] = { 0 };
                memcpy(iv, enc_ctx.Iv, AES_BLOCKLEN);

                //Encrypt message
                char encrypted[MESSAGE_LEN];
                memcpy(encrypted, msg, MESSAGE_LEN);
                AES_CBC_encrypt_buffer(&enc_ctx, (uint8_t*)encrypted, MESSAGE_LEN);
        
                send(client_fd, encrypted, MESSAGE_LEN, 0);

                mtx_lock(log_lock);
                add_log(message_log, encrypted, "SELF", iv);
                *new_message = 1;
                mtx_unlock(log_lock);

                memset(msg, '\0', MESSAGE_LEN);
                ind = 0;
            }else if(curr == '\b' || curr == 127 || curr == KEY_BACKSPACE){
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
                AES_init_ctx_iv(&dec_ctx, aes_key, curr->iv);
                char decrypted_msg[MESSAGE_LEN];
                memcpy(decrypted_msg, curr->msg, MESSAGE_LEN);
                AES_CBC_decrypt_buffer(&dec_ctx, (uint8_t*)decrypted_msg, MESSAGE_LEN);

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

    thrd_exit(THRDEXIT);
    return THRDEXIT;
}