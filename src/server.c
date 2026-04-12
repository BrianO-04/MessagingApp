#include "server.h"
#include "macros.h"
#include "user.h"
#include "hashmap.h"

#include <asm-generic/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// GLOBAL VARIABLES
int running = 1;
int opt = 1;
int server_fd;

struct sockaddr_in address;
socklen_t addrlen = sizeof(address);

pthread_t client_threads[MAX_CLIENTS] = { 0 };
int client_count = 0;

struct User** users;

// Mutex
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t join_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]){
    char buffer[1024] = { 0 };

    users = malloc((sizeof(struct User*)) * MAX_CLIENTS);
    for(int i = 0; i < MAX_CLIENTS; i++){
        users[i] = NULL;
    }

    // Create socket FD
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to initialize socket");
        exit(EXIT_FAILURE);
    }else{
        printf("Socket initialized\n");
    }

    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to port
    if(bind(server_fd, (struct sockaddr*)&address, addrlen) < 0){
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if(listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    pthread_t join_thread;

    
    pthread_create(&join_thread, NULL, connection_listen, NULL);

    pthread_join(join_thread, NULL);

    // Wait for all client threads to exit
    for(int i = 0; i < client_count; i++){
        pthread_join(client_threads[i], NULL);
    }

    close(server_fd);

    for(int i = 0; i < MAX_CLIENTS; i++){
        if(users[i] != NULL){
            free(users[i]);
        }
    }
    free(users);

    return EXIT_SUCCESS;
}

void* connection_listen(void* arg){
    while(1){
        // Accept a single connection
        int new_socket;
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&join_mutex);

        pthread_t client_thread;

        char* clientID = malloc(sizeof(char) * USERNAME_LEN);

        // Get username
        char nameBuffer[USERNAME_LEN] = { 0 };
        ssize_t valread = read(new_socket, nameBuffer, USERNAME_LEN);
        nameBuffer[USERNAME_LEN-1] = '\0';
        printf("%s joined the chat\n", nameBuffer);

        // Assign values
        struct User* new_user = malloc(sizeof(struct User));
        new_user->username = malloc(sizeof(char) * USERNAME_LEN);

        strcpy(new_user->username, nameBuffer);
        strcpy(clientID, nameBuffer);

        new_user->next = NULL;
        new_user->socket = new_socket;

        // Insert into users array
        put(new_user->username, new_user, users);

        client_count++;

        fflush(stdout);

        pthread_create(&client_thread, NULL, client_listen, clientID);

        pthread_mutex_unlock(&join_mutex);
    }

    pthread_exit(NULL);
    return NULL;
}

void* client_listen(void* arg){
    char client_id[USERNAME_LEN];
    strcpy(client_id, (char*)arg);
    free(arg);

    char msgBuffer[1024] = { 0 };

    int client_running = 1;

    struct User* user = get(client_id, users);

    char joinMSG[USERNAME_LEN + MESSAGE_LEN];
    snprintf(joinMSG, sizeof(joinMSG), "%s joined the chat\n", user->username);
    send_to_all(user->username, joinMSG, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));

    while(client_running){
        // Read incoming message
        // Read message text
        ssize_t valread = read(user->socket, msgBuffer, MESSAGE_LEN);
        msgBuffer[MESSAGE_LEN-1] = '\0';

        char final[USERNAME_LEN+MESSAGE_LEN];
        snprintf(final, sizeof(final), "%s: %s", user->username, msgBuffer);

        
        pthread_mutex_lock(&print_mutex);
        printf("%s", final);
        pthread_mutex_unlock(&print_mutex);

        if(strcmp(msgBuffer, "/EXIT\n") == 0){ // Client Disconnect Command
            client_running = 0;
            char msg[USERNAME_LEN + MESSAGE_LEN];
            snprintf(msg, sizeof(msg), "%s has disconnected\n", user->username);
            send_to_all(client_id, msg, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }else if(strcmp(msgBuffer, "/list\n") == 0){ // List active users
            char user_list[USERNAME_LEN + MESSAGE_LEN] = { 0 };
            strcat(user_list, "Connected Users:\n");

            for(int i = 0; i < MAX_CLIENTS; i++){
                if(users[i] != NULL){
                    struct User* curr = users[i];
                    while(curr != NULL){
                        if(strcmp(curr->username, user->username) != 0){
                            strcat(user_list, curr->username);
                            strcat(user_list, ", ");
                        }
                        curr = curr->next;
                    }
                }
            }

            strcat(user_list, "\n\n");

            send_to_ID(client_id, user_list, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }
        else{ // Normal message
            send_to_all(client_id, final, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }
        
    }

    pthread_mutex_lock(&join_mutex);

    delete(client_id, users);

    pthread_mutex_unlock(&join_mutex);

    pthread_exit(NULL);
    return NULL;
}

void send_to_all(char* sender_id, char* msg, size_t size){
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(users[i] != NULL){
            struct User* curr = users[i];
            while(curr != NULL){
                if(strcmp(curr->username, sender_id) != 0){
                    send(curr->socket, msg, size, 0);
                }
                curr = curr->next;
            }
        }
    }
}

void send_to_ID(char* client_id, char* msg, size_t size){
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(users[i] != NULL){
            struct User* curr = users[i];
            while(curr != NULL){
                if(strcmp(curr->username, client_id) == 0){
                    send(curr->socket, msg, size, 0);
                    return;
                }
                curr = curr->next;
            }
        }
    }
}