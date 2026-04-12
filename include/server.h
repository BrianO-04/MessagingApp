#ifndef SERVER_H_
#define SERVER_H_

#include "macros.h"

int main(int argc, char *argv[]);

// Listen for new clients
void* connection_listen(void* arg);

// Listen to clients
void* client_listen(void* arg);

#endif