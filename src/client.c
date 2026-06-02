#include "client.h"
#include "aes.h"
#include "macros.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cursesUI.h"


// GLOBAL VARIABLES
SOCKET client_fd;

int status;
struct sockaddr_in server_addr;

int *new_message;
int ui_initialized;
struct log* message_log;
int client_active = 1;

mtx_t log_lock;

int main(int argc, char *argv[]){

    #if defined(_WIN32)
    // WSADATA startup required for windows sockets
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }
    #endif

    mtx_init(&log_lock, mtx_plain);

    // TINY AES SETUP
    struct AES_ctx aes_ctx;
    uint8_t iv[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&aes_ctx, aes_key, iv);

    // Message log setup
    message_log = malloc(sizeof(struct log));
    init_log(message_log);

    char buffer[1024] = { 0 };

    if(argc != 3){
        printf("Expected usage: ./MessagingApp {name} {IP}\n");
        return EXIT_FAILURE;
    }

    char* uname = argv[1];
    char* ip = argv[2];

    #if defined(_WIN32) //Windows socket setup and error reporting
    if((client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET){
        printf("Failed to create socket\n");
        WSACleanup();
        return -1;
    }
    #else // POSIX systems
    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to create socket");
        return -1;
    }
    #endif

    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert text address to binary
    if(inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0){
        printf("Invalid address\n");
        return -1;
    }

    // Connect to the server
    if((status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0){
        printf("Connection failed");
        return -1;
    }
    cmd_types join_cmd = JOIN;
    send(client_fd, &join_cmd, sizeof(cmd_types), 0);
    send(client_fd, uname, sizeof(char) * USERNAME_LEN, 0);

    cmd_types join_conf = EMPTY;
    int valread = read_mp(client_fd, &join_conf, sizeof(join_conf));
    if(join_conf != USR_CONF){
        client_active = 0;
        printf("Kicked from server!\n");
    }

    // Initialize shared variables for UI
    new_message = malloc(sizeof(int));
    ui_initialized = 1;
    *new_message = 0;

    // Create array with shared variable pointers
    void **ui_args = malloc(sizeof(void*)*5);
    ui_args[0] = new_message;
    ui_args[1] = message_log;
    ui_args[2] = &log_lock;
    ui_args[3] = &client_fd;
    ui_args[4] = uname;

    thrd_t ui_thread;
    thrd_create(&ui_thread, init_ui, ui_args);

    thrd_t messaging_thread;
    thrd_create(&messaging_thread, server_listen, NULL);

    while(client_active){
        // Only use terminal input when there is no UI
        if(ui_initialized == 0){
            char message[MESSAGE_LEN];
            memset(message, 0, MESSAGE_LEN);
            fgets(message, sizeof(message), stdin);

            if(strcmp(message, "/EXIT\n") == 0){ // Disconnect Command
                cmd_types ext_cmd = USR_EXIT;
                send(client_fd, &ext_cmd, sizeof(cmd_types), 0);
                client_active = 0;
            }else {
                //Send message
                cmd_types msg_cmd = MESSAGE;
                send(client_fd, &msg_cmd, sizeof(cmd_types), 0);

                send(client_fd, aes_ctx.Iv, AES_BLOCKLEN, 0);
                //Encrypt message
                AES_CBC_encrypt_buffer(&aes_ctx, (uint8_t*)message, MESSAGE_LEN);
                
                send(client_fd, message, sizeof(char) * MESSAGE_LEN, 0);
            }
        }
    }

    // Probably need to figure this out later but that thread doesn't want to exit
    // thrd_join(messaging_thread, NULL);

    closesocket(client_fd);

    free(ui_args);
    free(new_message);
    free_log(message_log);
    free(message_log);
    
    return 0;
}

THRDFUNC server_listen(void* arg){
    struct AES_ctx aes_ctx;
    uint8_t iv[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&aes_ctx, aes_key, iv);

    while(client_active){
        char msg_buffer[MESSAGE_LEN] = { 0 };
        memset(msg_buffer, 0, MESSAGE_LEN);

        char usr_buffer[USERNAME_LEN] = { 0 };
        memset(usr_buffer, 0, USERNAME_LEN);

        // Read message IV from server
        uint8_t iv[AES_BLOCKLEN] = { 0 };
        int valread = read_mp(client_fd, iv, AES_BLOCKLEN);

        // Read username from server
        valread = read_mp(client_fd, usr_buffer, USERNAME_LEN);

        // Read encrypted message from server
        valread = read_mp(client_fd, msg_buffer, MESSAGE_LEN);

        AES_init_ctx_iv(&aes_ctx, aes_key, iv);
        char decrypted_msg[MESSAGE_LEN];
        memcpy(decrypted_msg, msg_buffer, MESSAGE_LEN);
        AES_CBC_decrypt_buffer(&aes_ctx, (uint8_t*)decrypted_msg, MESSAGE_LEN);
        
        decrypted_msg[MESSAGE_LEN-1] = '\0';

        char final[USERNAME_LEN+MESSAGE_LEN+2];
        snprintf(final, sizeof(final), "%s: %s", usr_buffer, decrypted_msg);

        if(ui_initialized == 0){
            printf("%s", final);
        }


        mtx_lock(&log_lock);

        add_log(message_log, msg_buffer, usr_buffer, iv);
        *new_message = 1;

        mtx_unlock(&log_lock);
    }
    thrd_exit(THRDEXIT);
    return THRDEXIT;
}