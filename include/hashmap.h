#ifndef HASH_H_
#define HASH_H_

#include "user.h"
#include <math.h>

#define MAX_CLIENTS 64

int put(char* key, struct User* user, struct User** hash_table);
struct User* get(char* key, struct User** hash_table);
int remove(char* key, struct User** hash_table);

#endif // HASH_H_