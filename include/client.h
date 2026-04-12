#ifndef CLIENT_H_
#define CLIENT_H_

#include "macros.h"

int initialize_client(char *uname, char *ip);

void* server_listen(void* arg);

#endif