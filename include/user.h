#ifndef USER_H_
#define USER_H_

#if defined(_WIN32)
#include <winsock2.h>
#endif

struct User{
    char* username;
    #if defined(_WIN32)
    SOCKET socket;
    #else
    int socket;
    #endif

    
    // Linked list thing for hash map
    struct User* next;
    struct User* last;
};

#endif // USER_H_