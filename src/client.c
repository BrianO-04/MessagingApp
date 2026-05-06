#include "client.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// GLOBAL VARIABLES
SOCKET client_fd;

int status;
int client_active = 1;
struct sockaddr_in server_addr;

int main(int argc, char *argv[]){

    #if defined(_WIN32)
    // WSADATA startup required for windows sockets
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }
    #endif

    char buffer[1024] = { 0 };

    if(argc != 3){
        printf("Expected usage: ./MessagingApp {name} {IP}\n");
        return EXIT_FAILURE;
    }

    char* uname = argv[1];
    char* ip = argv[2];

    #if defined(_WIN32) //Windows socket setup and error reporting
    if((client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET){
        printf("Failed to create socket\n");
        WSACleanup();
        return -1;
    }
    #else // POSIX systems
    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to create socket");
        return -1;
    }
    #endif

    
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
    cmd_types join_cmd = JOIN;
    send(client_fd, &join_cmd, sizeof(cmd_types), 0);
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

    closesocket(client_fd);
    
    return 0;
}

THRDFUNC server_listen(void* arg){
    while(client_active){
        char buffer[1024] = { 0 };
        int valread = read_mp(client_fd, buffer, 1024);
        buffer[USERNAME_LEN+MESSAGE_LEN-1] = '\0';
        printf("%s", buffer);
    }
    thrd_exit(THRDEXIT);
    return THRDEXIT;
}