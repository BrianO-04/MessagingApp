#ifndef HASH_H_
#define HASH_H_

#include "user.h"

int put(char* key, struct User* user, struct User** hash_table);
struct User* get(char* key, struct User** hash_table);
int remove(char* key, struct User** hash_table);

#endif // HASH_H_