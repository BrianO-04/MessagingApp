#include "client.h"
#include "message.h"

#include <asm-generic/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
int initialize_client(char *uname){

    int client_fd;
    int status;

    int running = 1;

    struct sockaddr_in server_addr;

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

    while(running){
        char message[MESSAGE_LEN];
        printf("Enter Text: ");
        fgets(message, sizeof(message), stdin);

        //Send Username
        send(client_fd, uname, sizeof(char) * USERNAME_LEN, 0);
        //Send message
        send(client_fd, message, sizeof(char) * MESSAGE_LEN, 0);
    }


    close(client_fd);
    return 0;
}