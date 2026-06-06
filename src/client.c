#include "client.h"
#include "aes.h"
#include "macros.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// GLOBAL VARIABLES
SOCKET client_fd;

int status;
struct sockaddr_in server_addr;

int new_message;
struct log* message_log;
int client_active = 1;

mtx_t log_lock;

// Plain Terminal UI for testing
int main(int argc, char *argv[]){
    int status = client_init(argv[1], argv[2]);
    if(status != 0){
        printf("Failed to initialize client\n");
        return -1;
    }

    struct AES_ctx aes_ctx;
    uint8_t iv[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&aes_ctx, aes_key, iv);

    while(client_active){
        mtx_lock(&log_lock);
        if(new_message == 1){
            AES_init_ctx_iv(&aes_ctx, aes_key, message_log->tail->iv);

            char decrypted_msg[MESSAGE_LEN];
            memcpy(decrypted_msg, message_log->tail->msg, MESSAGE_LEN);
            AES_CBC_decrypt_buffer(&aes_ctx, (uint8_t*)decrypted_msg, MESSAGE_LEN);

            printf("%s\n", decrypted_msg);
        }
        new_message = 0;
        mtx_unlock(&log_lock);
    }

    return 0;
}

int client_init(char* uname, char* ip){
    #if defined(_WIN32)
    // WSADATA startup required for windows sockets
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }
    #endif

    mtx_init(&log_lock, mtx_plain);

    // Message log setup
    message_log = malloc(sizeof(struct log));
    init_log(message_log);

    char buffer[1024] = { 0 };

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

    thrd_t messaging_thread;
    thrd_create(&messaging_thread, server_listen, NULL);
    
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

        mtx_lock(&log_lock);

        add_log(message_log, msg_buffer, usr_buffer, iv);
        new_message = 1;

        mtx_unlock(&log_lock);
    }
    thrd_exit(THRDEXIT);
    return THRDEXIT;
}