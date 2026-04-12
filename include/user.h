#ifndef USER_H_
#define USER_H_

struct User{
    char username[16];
    int socket;

    // Linked list thing for hash map
    struct User* next;
};

#endif // USER_H_