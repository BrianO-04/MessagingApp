#include "client.h"

#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char *argv[]){
    char buffer[1024] = { 0 };

    if(argc != 3){
        printf("Expected usage: ./MessagingApp {name} {IP}\n");
        return EXIT_FAILURE;
    }

    char* uname = argv[1];
    char* ip = argv[2];

    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to create socket");
        return -1;
    }
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
    send(client_fd, uname, sizeof(char) * USERNAME_LEN, 0);

    thrd_t messaging_thread;
    thrd_create(&messaging_thread, server_listen, NULL);

    while(client_active){
        char message[MESSAGE_LEN];
        fgets(message, sizeof(message), stdin);

        //Send message
        send(client_fd, message, sizeof(char) * MESSAGE_LEN, 0);

        if(strcmp(message, "/EXIT\n") == 0){ // Disconnect Command
            client_active = 0;
        }
    }

    // Probably need to figure this out later but that thread doesn't want to exit
    // thrd_join(messaging_thread, NULL);


    close(client_fd);
    return 0;
}


int server_listen(void* arg){
    char buffer[1024] = { 0 };

    while(client_active){
        ssize_t valread = read(client_fd, buffer, USERNAME_LEN+MESSAGE_LEN);
        buffer[USERNAME_LEN+MESSAGE_LEN-1] = '\0';
        printf("%s", buffer);
    }

    thrd_exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}