#include "hashmap.h"
#include "macros.h"
#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

    printf("User not found, returning null\n");
    return NULL;
}

int delete(char* key, struct User** hash_table){

    int hash_index = hash(key);
    
    if(hash_table[hash_index] == NULL){
        return 1;
    }else if(hash_table[hash_index]->next == NULL){
        free(hash_table[hash_index]);
        hash_table[hash_index] = NULL;
        return 1;
    }else{
        struct User* curr = hash_table[hash_index];
        hash_table[hash_index] = curr->next;
        free(curr->username);
        free(curr);
        return 1;
    }

    return 0;
}