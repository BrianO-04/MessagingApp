#include "client.h"

#include <asm-generic/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// GLOBAL VARIABLES
int client_fd;
int status;
int client_active = 1;
struct sockaddr_in server_addr;

int initialize_client(char *uname){
    char buffer[1024] = { 0 };

    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to create socket");
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert text address to binary
    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0){
        printf("Invalid address\n");
        return -1;
    }

    // Connect to the server
    if((status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0){
        printf("Connection failed");
        return -1;
    }

    pthread_t messaging_thread;
    pthread_create(&messaging_thread, NULL, server_listen, NULL);

    while(client_active){
        char message[MESSAGE_LEN];
        fgets(message, sizeof(message), stdin);

        //Send Username
        send(client_fd, uname, sizeof(char) * USERNAME_LEN, 0);
        //Send message
        send(client_fd, message, sizeof(char) * MESSAGE_LEN, 0);

        if(strcmp(message, "/EXIT\n") == 0){ // SERVER SHUTDOWN COMMAND
            client_active = 0;
        }
    }


    close(client_fd);
    return 0;
}


void* server_listen(void* arg){
    char buffer[1024] = { 0 };

    while(client_active){
        ssize_t valread = read(client_fd, buffer, USERNAME_LEN+MESSAGE_LEN);
        buffer[USERNAME_LEN+MESSAGE_LEN-1] = '\0';
        printf("%s", buffer);
    }

    pthread_exit(NULL);
    return NULL;
}