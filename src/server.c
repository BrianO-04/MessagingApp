#include "server.h"
#include "message.h"

#include <asm-generic/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 32

// GLOBAL VARIABLES
int running = 1;
int opt = 1;
int server_fd;

struct sockaddr_in address;
socklen_t addrlen = sizeof(address);

int connections[MAX_CLIENTS] = { 0 };
pthread_t client_threads[MAX_CLIENTS] = { 0 };
int client_count = 0;

// Mutex
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t join_mutex = PTHREAD_MUTEX_INITIALIZER;

int initialize_server(){
    char buffer[1024] = { 0 };

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

    // Accept a single connection
    // int new_socket;
    // if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
    //     perror("accept");
    //     exit(EXIT_FAILURE);
    // }

    // while(running){
    //     // Read incoming message
    //     // Read username
    //     ssize_t valread = read(new_socket, buffer, USERNAME_LEN);
    //     buffer[USERNAME_LEN-1] = '\0';
    //     printf("%s: ", buffer);

    //     // Read message text
    //     valread = read(new_socket, buffer, MESSAGE_LEN);
    //     buffer[MESSAGE_LEN-1] = '\0';
    //     printf("%s", buffer);

    //     if(strcmp(buffer, "/EXIT\n") == 0){ // SERVER SHUTDOWN COMMAND
    //         running = 0;
    //     }
    // }

    pthread_t join_thread;

    pthread_create(&join_thread, NULL, connection_listen, NULL);

    pthread_join(join_thread, NULL);

    // Wait for all client threads to exit
    for(int i = 0; i < client_count; i++){
        pthread_join(client_threads[i], NULL);
    }

    close(server_fd);
    return 0;
}

void* connection_listen(void* arg){

    while(running){
        // Accept a single connection
        int new_socket;
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&join_mutex);

        connections[client_count] = new_socket;

        pthread_t client_thread;

        int* clientID = malloc(sizeof(int));
        *clientID = client_count++; 

        pthread_create(&client_thread, NULL, client_listen, clientID);

        pthread_mutex_unlock(&join_mutex);
    }

    pthread_exit(NULL);
    return NULL;
}

void* client_listen(void* arg){
    int client_id = *(int*)arg;
    free(arg);
    char nameBuffer[1024] = { 0 };
    char msgBuffer[1024] = { 0 };

    while(running){
        // Read incoming message
        // Read username
        ssize_t valread = read(connections[client_id], nameBuffer, USERNAME_LEN);
        nameBuffer[USERNAME_LEN-1] = '\0';

        // Read message text
        valread = read(connections[client_id], msgBuffer, MESSAGE_LEN);
        msgBuffer[MESSAGE_LEN-1] = '\0';

        char final[USERNAME_LEN+MESSAGE_LEN];
        snprintf(final, sizeof(final), "%s: %s", nameBuffer, msgBuffer);

        
        pthread_mutex_lock(&print_mutex);
        printf("%s", final);
        pthread_mutex_unlock(&print_mutex);

        for(int i = 0; i < client_count; i++){
            if(i != client_id)
                send(connections[i], final, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN), 0);
        }

        if(strcmp(msgBuffer, "/EXIT\n") == 0){ // SERVER SHUTDOWN COMMAND
            running = 0;
        }
    }

    pthread_exit(NULL);
    return NULL;
}