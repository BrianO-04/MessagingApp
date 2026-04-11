#include "server.h"
#include "message.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#define PORT 8080
int initialize_server(){

    int server_fd;

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to initialize socket");
        exit(EXIT_FAILURE);
    }else{
        printf("Socket initialized\n");
    }

    close(server_fd);
    return 0;
}