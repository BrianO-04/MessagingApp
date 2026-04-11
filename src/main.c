#include "server.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){


    if(argc == 1){
        //INIT CLIENT
        printf("Initializing Client\n");
        return initialize_client();
    }else if (argc == 2) {
        if(strcmp("server", argv[1]) == 0){
            //INIT SERVER
            printf("Initializing Server\n");
            return initialize_server();
        }else{ 
            //INIT CLIENT
            printf("Initializing Client\n");
            return initialize_client();
        }
    }else {
        printf("Expected one argument!\nUsage: ./MessagingApp client OR ./MessagingApp server\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}