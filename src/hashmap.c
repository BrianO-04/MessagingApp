#include "hashmap.h"
#include "macros.h"
#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

// Polynomial Rolling Hash Function
int hash(char* key){
    int hashed = 0;
    int p = 53;

    int i = 0;
    while(key[i] != '\0'){
        hashed = (hashed * p + key[i]) % MAX_CLIENTS;
        i++;
    }

    return hashed % MAX_CLIENTS;
}

int put(char* key, struct User* user, struct User** hash_table){

    int hash_index = hash(key);

    if(hash_table[hash_index] == NULL){
        hash_table[hash_index] = user;
        return 1;
    }

    struct User* curr = hash_table[hash_index];
    while(curr->next != NULL){
        curr = curr->next;
    }
    curr->next = user;
    user->last = curr;

    return 1;
}

struct User* get(char* key, struct User** hash_table){

    int hash_index = hash(key);

    struct User* curr = hash_table[hash_index];
    while(curr != NULL){
        if(strcmp(key, curr->username) == 0){
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

int delete(char* key, struct User** hash_table){
    int hash_index = hash(key);

    struct User *curr = hash_table[hash_index];

    while(curr != NULL && strcmp(curr->username, key) != 0)
        curr = curr->next;

    if(curr == NULL)
        return 0;

    if(curr->last){
        curr->last->next = curr->next;
    }
    else{
        hash_table[hash_index] = curr->next;
    }
    if(curr->next){
        curr->next->last = curr->last;
    }

    free(curr->username);
    free(curr);

    return 1;
}
