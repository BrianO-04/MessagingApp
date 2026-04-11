#ifndef CLIENT_H_
#define CLIENT_H_

#include "message.h"

int initialize_client(char *uname);

void* server_listen(void* arg);

#endif