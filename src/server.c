#include "server.h"
#include "macros.h"
#include "user.h"
#include "hashmap.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "messagelog.h"

// GLOBAL VARIABLES
int running = 1;
int opt = 1;

SOCKET server_fd;

struct sockaddr_in address;
socklen_t addrlen = sizeof(address);


int client_count = 0;

struct User** users;

// Mutex
#if defined(__APPLE__) && defined(__MACH__)
pthread_t client_threads[MAX_CLIENTS] = { 0 };
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
thrd_t client_threads[MAX_CLIENTS] = { 0 };
mtx_t print_mutex;
mtx_t hash_mutex;
#endif

struct log* message_log;

int main(int argc, char *argv[]){
    
    #if defined(_WIN32)
    // WSADATA startup required for windows sockets
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }
    #endif

    // TINY AES SETUP
    struct AES_ctx aes_ctx;
    uint8_t iv[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&aes_ctx, aes_key, iv);

    // Message log setup
    message_log = malloc(sizeof(struct log));
    init_log(message_log);

    #if !defined(__APPLE__) && !defined(__MACH__)
    // Initialize Mutex, required for threads.h
    mtx_init(&print_mutex, mtx_plain);
    mtx_init(&hash_mutex, mtx_plain);
    #endif

    // Create users hash table and set base values to NULL
    users = malloc((sizeof(struct User*)) * MAX_CLIENTS);
    for(int i = 0; i < MAX_CLIENTS; i++){
        users[i] = NULL;
    }

    // Create server socket file descriptor
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to initialize socket");
        exit(EXIT_FAILURE);
    }else{
        printf("Socket initialized\n");
    }

    #if defined(_WIN32)
    // Set socket options
    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt))) == SOCKET_ERROR){
        printf("Failed to set socket options\n");
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    #else
    // Set socket options
    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    #endif

    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to port
    if(bind(server_fd, (struct sockaddr*)&address, addrlen) < 0){
        #if defined(_WIN32)
        printf("Failed to bind port");
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
        #else
        perror("Failed to bind");
        exit(EXIT_FAILURE);
        #endif
    }

    // Start listening on socket
    if(listen(server_fd, 3) < 0){
        #if defined(_WIN32)
        printf("Failed to listen");
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
        #else
        perror("listen");
        exit(EXIT_FAILURE);
        #endif
    }

    #if defined(__APPLE__) && defined(__MACH__)
    // Create the thread that listens for incomming connections
    pthread_t new_connections_thread;
    pthread_create(&new_connections_thread, NULL, connection_listen, NULL);

    // Wait for the connection listening thread to finish
    pthread_join(new_connections_thread, NULL);

    // Wait for all client threads to finish executing before closing socket
    for(int i = 0; i < client_count; i++){
        pthread_join(client_threads[i], NULL);
    }
    #else
    // Create the thread that listens for incomming connections
    thrd_t new_connections_thread;
    thrd_create(&new_connections_thread, connection_listen, NULL);

    // Wait for the connection listening thread to finish
    thrd_join(new_connections_thread, NULL);

    // Wait for all client threads to finish executing before closing socket
    for(int i = 0; i < client_count; i++){
        thrd_join(client_threads[i], NULL);
    }
    #endif

    closesocket(server_fd);
    
    // Clean up memory
    // Free all usernames
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(users[i] != NULL){
            free(users[i]);
        }
    }
    // Free the users array
    free(users);

    #if defined(_WIN32)
    WSACleanup();
    #endif

    return EXIT_SUCCESS;
}

THRDFUNC connection_listen(void* arg){
    while(1){
        // Wait for a connection attempt
        #if defined(_WIN32)
        SOCKET new_socket;
        addrlen = sizeof(address);
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) == SOCKET_ERROR){
            printf("Failed to connect\n");
            exit(EXIT_FAILURE);
        }
        #else
        int new_socket;
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }
        #endif

        // Get username from client
        cmd_types joincmd = EMPTY;
        int valread = read_mp(new_socket, &joincmd, sizeof(cmd_types));
        if(joincmd != JOIN) return THRDFAIL;

        char namebuf[USERNAME_LEN] = { 0 };
        valread = read_mp(new_socket, namebuf, USERNAME_LEN);
        namebuf[USERNAME_LEN-1] = '\0';

        if(get(namebuf, users) != NULL || strcmp(namebuf, "[SERVER]") == 0){
            closesocket(new_socket);
            continue;
        }

        cmd_types confirm = USR_CONF;
        send(new_socket, &confirm, sizeof(cmd_types), 0);

        // Create User struct
        struct User* new_user = malloc(sizeof(struct User));
        new_user->username = malloc(sizeof(char) * USERNAME_LEN);
        #if defined(_WIN32)
        strcpy_s(new_user->username, USERNAME_LEN, namebuf);
        #else
        strcpy(new_user->username, namebuf);
        #endif

        new_user->next = NULL;
        new_user->socket = new_socket;

        #if defined(__APPLE__) && defined(__MACH__)
        pthread_mutex_lock(&hash_mutex);
        #else
        // Mutual exclusion, only one thread can modify the hash table at a time  
        mtx_lock(&hash_mutex);
        #endif

        
        
        put(new_user->username, new_user, users);
        client_count++;

        #if defined(__APPLE__) && defined(__MACH__)
        // Create a new thread for listening to that client's messages
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, client_listen, new_user);

        pthread_mutex_unlock(&hash_mutex);
        #else
        // Create a new thread for listening to that client's messages
        thrd_t client_thread;
        thrd_create(&client_thread, client_listen, new_user);

        mtx_unlock(&hash_mutex);
        #endif

        
    }

    thrd_exit(THRDEXIT);
    return THRDEXIT;
}

THRDFUNC client_listen(void* arg){
    // Set up AES
    struct AES_ctx aes_ctx;
    uint8_t iv[AES_BLOCKLEN] = {0};
    AES_init_ctx_iv(&aes_ctx, aes_key, iv);

    // Get user struct and username from passed in arg
    struct User* user = (struct User*)arg;
    char* client_id = user->username;

    // Create join message
    char joinMSG[MESSAGE_LEN];
    snprintf(joinMSG, sizeof(joinMSG), "%s joined the chat\n", client_id);

    // Encrypt join msg
    uint8_t join_iv[AES_BLOCKLEN] = {0};
    memcpy(join_iv, aes_ctx.Iv, AES_BLOCKLEN);
    AES_CBC_encrypt_buffer(&aes_ctx, (uint8_t*)joinMSG, MESSAGE_LEN);

    // Broadcast join message to all users
    add_log(message_log, joinMSG, "[SERVER]\0", join_iv);
    send_to_all(client_id, joinMSG, join_iv, 1);
    print_log(user->socket);

    // Start listening loop
    char msgBuffer[1024] = { 0 };
    int client_running = 1;
    while(client_running){

        memset(msgBuffer, 0, 1024);

        cmd_types incomming_type = EMPTY;
        int valread = read_mp(user->socket, &incomming_type, sizeof(cmd_types));

        if(valread <= 0 || incomming_type == USR_EXIT){ // Disconnect
            client_running = 0;
            char msg[MESSAGE_LEN];
            snprintf(msg, sizeof(msg), "%s has disconnected\n", client_id);

            // Encrypt join msg
            uint8_t dc_iv[AES_BLOCKLEN] = {0};
            memcpy(dc_iv, aes_ctx.Iv, AES_BLOCKLEN);
            AES_CBC_encrypt_buffer(&aes_ctx, (uint8_t*)msg, MESSAGE_LEN);

            add_log(message_log, msg, "[SERVER]\0", dc_iv);
            send_to_all(client_id, msg, dc_iv, 1);
            break;
        }

        // Read incomming Iv
        uint8_t newiv[AES_BLOCKLEN] = { 0 };
        valread = read_mp(user->socket, newiv, AES_BLOCKLEN);

        // Read incoming message
        valread = read_mp(user->socket, msgBuffer, MESSAGE_LEN);

        // Send message and add to log
        add_log(message_log, msgBuffer, client_id, newiv);
        send_to_all(client_id, msgBuffer, newiv, 0);
    }

    mtx_lock(&hash_mutex);

    closesocket(user->socket);
    delete(client_id, users);

    mtx_unlock(&hash_mutex);

    thrd_exit(THRDEXIT);
    return THRDEXIT;
}

void send_to_all(char* sender_id, char* msg, uint8_t* iv, int is_server){
    char cpy[MESSAGE_LEN];
    memcpy(cpy, msg, MESSAGE_LEN);
    
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(users[i] != NULL){
            struct User* curr = users[i];
            while(curr != NULL){
                if(strcmp(curr->username, sender_id) != 0){
                    send(curr->socket, iv, AES_BLOCKLEN, 0);
                    if(is_server){
                        send(curr->socket, "[SERVER]\0", USERNAME_LEN, 0);
                    }else{
                        send(curr->socket, sender_id, USERNAME_LEN, 0);
                    }
                    send(curr->socket, cpy, MESSAGE_LEN, 0);
                }
                curr = curr->next;
            }
        }
    }
}

void send_to_ID(char* client_id, char* msg, size_t size){
    struct User* target = get(client_id, users);
    send(target->socket, msg, strlen(msg), 0);
}

#if defined(_WIN32)
void print_log(SOCKET client){
#else
void print_log(int client){
#endif
    struct message* current = message_log->head;
    while(current != NULL){
        send(client, current->iv, AES_BLOCKLEN, 0);
        send(client, current->usr, USERNAME_LEN, 0);
        send(client, current->msg, MESSAGE_LEN, 0);
        current = current->next;
    }
}