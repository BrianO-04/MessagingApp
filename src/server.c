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

// Windows Sockets
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <BaseTsd.h>
#else // Posix sockets
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GLOBAL VARIABLES
int running = 1;
int opt = 1;

#if defined(_WIN32)
// Not a file descriptor but keeping the name the same for consistency
SOCKET server_fd;
#else
int server_fd;
#endif

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

// Message Log
int head = 0;
char msgLog[MAXLOG][MESSAGE_LEN+USERNAME_LEN];

int main(int argc, char *argv[]){
    
    #if defined(_WIN32)
    // WSADATA startup required for windows sockets
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }
    #endif

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

    #if defined(_WIN32) // Windows Socket Close
    closesocket(server_fd);
    #else // POSIX socket close
    close(server_fd);
    #endif
    
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

#if defined(__APPLE__) && defined(__MACH__)
void* connection_listen(void* arg){
#else
int connection_listen(void* arg){
#endif
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
        char namebuf[USERNAME_LEN] = { 0 };
        #if defined(_WIN32)
        int valread = recv(new_socket, namebuf, USERNAME_LEN, 0);
        #else
        ssize_t valread = read(new_socket, namebuf, USERNAME_LEN);
        #endif

        namebuf[USERNAME_LEN-1] = '\0';
        printf("%s joined the chat\n", namebuf);

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
        #if defined(_WIN32)
        int valread = recv(user->socket, msgBuffer, MESSAGE_LEN, 0);
        #else
        ssize_t valread = read(user->socket, msgBuffer, MESSAGE_LEN);
        #endif

        if(valread <= 0){ // Disconnect
            client_running = 0;
            char msg[USERNAME_LEN + MESSAGE_LEN];
            snprintf(msg, sizeof(msg), "%s has disconnected\n", user->username);
            //printf("%s", msg);
            print_msg(msg);
            send_to_all(client_id, msg, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
            break;
        }

        msgBuffer[MESSAGE_LEN-1] = '\0';

        // Combine Username: Message
        char final[USERNAME_LEN+MESSAGE_LEN];
        snprintf(final, sizeof(final), "%s: %s", user->username, msgBuffer);

        // Lock Mutex
        #if defined(__APPLE__) && defined(__MACH__)
        pthread_mutex_lock(&print_mutex);
        #else
        mtx_lock(&print_mutex);
        #endif

        //printf("%s", final);
        print_msg(final);
        
        // Unlock Mutex
        #if defined(__APPLE__) && defined(__MACH__)
        pthread_mutex_unlock(&print_mutex);
        #else
        mtx_unlock(&print_mutex);
        #endif
        

        // Check if the message is a valid command
        if(strcmp(msgBuffer, "/EXIT\n") == 0){ // Client Disconnect Command
            client_running = 0;
            char msg[USERNAME_LEN + MESSAGE_LEN];
            snprintf(msg, sizeof(msg), "%s has disconnected\n", user->username);
            print_msg(msg);
            send_to_all(client_id, msg, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }else if(strcmp(msgBuffer, "/list\n") == 0){ // List active users
            char user_list[USERNAME_LEN + MESSAGE_LEN] = { 0 };
            #if defined(_WIN32)
            strcat_s(user_list, sizeof(user_list), "Connected Users: ");
            #else
            strcat(user_list, "Connected Users: ");
            #endif

            for(int i = 0; i < MAX_CLIENTS; i++){
                if(users[i] != NULL){
                    struct User* curr = users[i];
                    while(curr != NULL){
                        if(strcmp(curr->username, user->username) != 0){
                            #if defined(_WIN32)
                            strcat_s(user_list, sizeof(user_list), curr->username);
                            strcat_s(user_list, sizeof(user_list), ", ");
                            #else
                            strcat(user_list, curr->username);
                            strcat(user_list, ", ");
                            #endif
                        }
                        curr = curr->next;
                    }
                }
            }

            #if defined(_WIN32)
            strcat_s(user_list, sizeof(user_list), "\n");
            #else
            strcat(user_list, "\n");
            #endif

            send_to_ID(client_id, user_list, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }else if(strcmp(msgBuffer, "/logtest\n") == 0){
            print_log();
        }
        else{ // Normal message, send to all users
            send_to_all(client_id, final, sizeof(char) * (USERNAME_LEN + MESSAGE_LEN));
        }
        
    }

    // If the loop has exited, the client has disconnected
    // Lock the hash mutex and delete the user
    #if defined(__APPLE__) && defined(__MACH__)
    pthread_mutex_lock(&hash_mutex);
    delete(client_id, users);
    pthread_mutex_unlock(&hash_mutex);

    pthread_exit(NULL);
    return NULL;
    #else
    mtx_lock(&hash_mutex);

    #if defined(_WIN32)
    closesocket(user->socket);
    #else
    close(user->socket);
    #endif

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
                    send(curr->socket, msg, strlen(msg), 0);
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

void print_msg(char* msg){
    printf("%s", msg);
    strcpy(msgLog[head], msg);
    head = (head+1) % MAXLOG;
}

void print_log(){
    printf("PRINTING LOG\n");
    for(int i = 0; i < MAXLOG; i++){
        int j = (i + head) % MAXLOG;
        printf("%s\n", msgLog[j]);
    }
    printf("LOG PRINTED\n");
}