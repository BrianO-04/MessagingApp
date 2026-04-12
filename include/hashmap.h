#ifndef HASH_H_
#define HASH_H_

#include "user.h"
#include <math.h>

#define MAX_CLIENTS 64

// Inserts a user into the hash table
// Returns 1 on success
int put(char* key, struct User* user, struct User** hash_table);

// Gets a user from the hash table using the username as a key
// Returns NULL if the user is not found
struct User* get(char* key, struct User** hash_table);

// Removes a user from the hash table and frees the user struct
// Returns 1 on success and 0 on failure
int delete(char* key, struct User** hash_table);

#endif // HASH_H_