#include "client.h"

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
#if defined(_WIN32)
// Not a file descriptor but keeping the name the same for consistency
SOCKET client_fd;
#else
int client_fd;
#endif

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
    send(client_fd, uname, sizeof(char) * USERNAME_LEN, 0);

    #if defined(__APPLE__) && defined(__MACH__)
    pthread_t messaging_thread;
    pthread_create(&messaging_thread, NULL, server_listen, NULL);
    #else
    thrd_t messaging_thread;
    thrd_create(&messaging_thread, server_listen, NULL);
    #endif

    

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

    #if defined(_WIN32) // Windows Socket Close
    closesocket(client_fd);
    #else // POSIX socket close
    close(client_fd);
    #endif
    
    return 0;
}


#if defined(__APPLE__) && defined(__MACH__)
void* server_listen(void* arg){
#else
int server_listen(void* arg){
#endif
    char buffer[1024] = { 0 };

    while(client_active){
        #if defined(_WIN32)
        SSIZE_T valread = recv(client_fd, buffer, USERNAME_LEN+MESSAGE_LEN, 0);
        #else
        ssize_t valread = read(client_fd, buffer, USERNAME_LEN+MESSAGE_LEN);
        #endif

        buffer[USERNAME_LEN+MESSAGE_LEN-1] = '\0';
        printf("%s", buffer);
    }
    #if defined(__APPLE__) && defined(__MACH__)
    pthread_exit(NULL);
    return NULL;
    #else
    thrd_exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
    #endif
}