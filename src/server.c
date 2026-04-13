#include "server.h"
#include "macros.h"
#include "user.h"
#include "hashmap.h"

// MacOS does not support threads.h so use pthread.h instead
#if defined(__APPLE__) && defined(__MACH__)
#include <pthread.h>
#else
#include <threads.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// GLOBAL VARIABLES
int running = 1;
int opt = 1;
int server_fd;

struct sockaddr_in address;
socklen_t addrlen = sizeof(address);

thrd_t client_threads[MAX_CLIENTS] = { 0 };
int client_count = 0;

struct User** users;

// Mutex
#if defined(__APPLE__) && defined(__MACH__)
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
mtx_t print_mutex;
mtx_t hash_mutex;
#endif

#if defined(__APPLE__) && defined(__MACH__)

#else

#endif

int main(int argc, char *argv[]){
    
    #if !defined(__APPLE__) && !defined(__MACH__)
    // Initialize Mutex
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

    // Set socket options
    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) < 0){
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

    // Start listening on socket
    if(listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
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
    
    close(server_fd);

    // Clean up memory
    // Free all usernames
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(users[i] != NULL){
            free(users[i]);
        }
    }
    // Free the users array
    free(users);

    return EXIT_SUCCESS;
}

#if defined(__APPLE__) && defined(__MACH__)
void* connection_listen(void* arg){
#else
int connection_listen(void* arg){
#endif
    while(1){
        // Wait for a connection attempt
        int new_socket;
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        // Get username from client
        char namebuf[USERNAME_LEN] = { 0 };
        ssize_t valread = read(new_socket, namebuf, USERNAME_LEN);
        namebuf[USERNAME_LEN-1] = '\0';
        printf("%s joined the chat\n", namebuf);

        // Create User struct
        struct User* new_user = malloc(sizeof(struct User));
        new_user->username = malloc(sizeof(char) * USERNAME_LEN);
        strcpy(new_user->username, namebuf);
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

    #if defined(__APPLE__) && defined(__MACH__)
    pthread_exit(NULL);
    return NULL;
    #else
    thrd_exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
    #endif
}

#if defined(__APPLE__) && defined(__MACH__)
void* client_listen(void* arg){
#else
int client_listen(void* arg){
#endif
    // Get user struct and username from passed in arg
    struct User* user = (struct User*)arg;
    char* client_id = user->username;

    // Broadcast join message to all users
    char joinMSG[USERNAME_LEN + MESSAGE_LEN];
    snprintf(joinMSG, sizeof(joinMSG), "%s joined the chat\n", client_id);
    send_to_all(client_id, joinMSG, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));

    // Start listening loop
    char msgBuffer[1024] = { 0 };
    int client_running = 1;
    while(client_running){

        // Read incoming message
        ssize_t valread = read(user->socket, msgBuffer, MESSAGE_LEN);
        msgBuffer[MESSAGE_LEN-1] = '\0';

        // Combine Username: Message
        char final[USERNAME_LEN+MESSAGE_LEN];
        snprintf(final, sizeof(final), "%s: %s", user->username, msgBuffer);

        #if defined(__APPLE__) && defined(__MACH__)
        pthread_mutex_lock(&print_mutex);
        printf("%s", final);
        pthread_mutex_unlock(&print_mutex);
        #else
        // Lock the printing mutex before printing the message
        mtx_lock(&print_mutex);
        printf("%s", final);
        mtx_unlock(&print_mutex);
        #endif
        

        // Check if the message is a valid command
        if(strcmp(msgBuffer, "/EXIT\n") == 0){ // Client Disconnect Command
            client_running = 0;
            char msg[USERNAME_LEN + MESSAGE_LEN];
            snprintf(msg, sizeof(msg), "%s has disconnected\n", user->username);
            send_to_all(client_id, msg, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }else if(strcmp(msgBuffer, "/list\n") == 0){ // List active users
            char user_list[USERNAME_LEN + MESSAGE_LEN] = { 0 };
            strcat(user_list, "Connected Users: ");

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

            strcat(user_list, "\n");

            send_to_ID(client_id, user_list, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }
        else{ // Normal message, send to all users
            send_to_all(client_id, final, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }
        
    }

    // If the loop has exited, the client has disconnected
    // Lock the hash mutex and delete the user
    #if defined(__APPLE__) && defined(__MACH__)
    // If the loop has exited, the client has disconnected
    // Lock the hash mutex and delete the user
    pthread_mutex_lock(&hash_mutex);
    delete(client_id, users);
    pthread_mutex_unlock(&hash_mutex);

    pthread_exit(NULL);
    return NULL;
    #else
    mtx_lock(&hash_mutex);
    delete(client_id, users);
    mtx_unlock(&hash_mutex);

    thrd_exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
    #endif
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
    struct User* target = get(client_id, users);
    send(target->socket, msg, size, 0);
}