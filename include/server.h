#ifndef SERVER_H_
#define SERVER_H_

#include "message.h"

int initialize_server();

// Listen for new clients
void* connection_listen(void* arg);

// Listen to clients
void* client_listen(void* arg);

#endif