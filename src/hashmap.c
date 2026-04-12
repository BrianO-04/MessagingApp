#include "hashmap.h"
#include "user.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HASHLEN 64

// Polynomial Rolling Hash Function
int hash(char* key){
    int hashed = 0;
    int p = 53;

    int i = 0;
    while(key[i] != '\0'){
        hashed += (int)key[i] * pow(p, i);
    }

    return hashed % HASHLEN;
}

int put(char* key, struct User* user, struct User** hash_table){

    int hash_index = hash(key);

    if(hash_table[hash_index] == NULL){
        hash_table[hash_index] = user;
    }else{
        struct User* next = hash_table[hash_index]->next;
        while(next != NULL){
            next = next->next;
        }
        next->next = user;
    }

    return 1;
}

struct User* get(char* key, struct User** hash_table){

    int hash_index = hash(key);

    if(hash_table[hash_index] == NULL){
        return NULL;
    }else if(strcmp(key, hash_table[hash_index]->username) == 0){
        return hash_table[hash_index];
    }else{
        struct User* next = hash_table[hash_index]->next;
        while(next != NULL){
            if(strcmp(key, next->username) == 0){
                return next;
            }
            next = next->next;
        }
        // If it wasn't found in that loop, it isn't in the table.
        return NULL;
    }

    return NULL;
}

int remove(char* key, struct User** hash_table){

    int hash_index = hash(key);
    
    if(hash_table[hash_index] == NULL){
        return NULL;
    }else if(hash_table[hash_index]->next == NULL){
        free(hash_table[hash_index]);
        hash_table[hash_index] = NULL;
        return 1;
    }else{
        struct User* curr = hash_table[hash_index];
        hash_table[hash_index] = curr->next;
        free(curr);
        return 1;
    }

    return 0;
}