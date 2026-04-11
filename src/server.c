#include "server.h"
#include "message.h"

#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
int initialize_server(){

    int server_fd;
    int opt = 1;

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

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
    int new_socket;
    if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Read incoming message
    // Read username
    ssize_t valread = read(new_socket, buffer, USERNAME_LEN-1);
    buffer[valread] = '\0';
    printf("%s: ", buffer);

    // Read message text
    valread = read(new_socket, buffer, MESSAGE_LEN-1);
    buffer[valread] = '\0';
    printf("%s\n", buffer);

    // Send message
    char* rec = "Message received";
    send(new_socket, rec, strlen(rec), 0);
    printf("Sending confirmation message");

    close(new_socket);
    close(server_fd);
    return 0;
}