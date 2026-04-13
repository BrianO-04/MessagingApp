#ifndef SERVER_H_
#define SERVER_H_

#include "macros.h"

int main(int argc, char *argv[]);

// Listen for new clients
int connection_listen(void* arg);

// Listen to clients
int client_listen(void* arg);

// Send a message to all connected clients
void send_to_all(char* sender_id, char* msg, size_t size);

// Send to specific user
void send_to_ID(char* client_id, char* msg, size_t size);

#endif