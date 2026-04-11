#include "server.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){

    if(argc == 2){
        if(strcmp(argv[1], "server") == 0){
            //INIT SERVER
            printf("Initializing Server\n");
            return initialize_server();
        }else{
            printf("Expected usage:\n./MessagingApp server\n./MessagingApp client {Username} {Message}");
            return EXIT_FAILURE;
        }
    }else if(argc == 4){
        if(strcmp(argv[1], "client") == 0){
            //INIT CLIENT
            printf("Initializing Client\n");
            return initialize_client();
        }else{
            printf("Expected usage:\n./MessagingApp server\n./MessagingApp client {Username} {Message}");
            return EXIT_FAILURE;
        }
    }else{
        printf("Expected usage:\n./MessagingApp server\n./MessagingApp client {Username} {Message}");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}