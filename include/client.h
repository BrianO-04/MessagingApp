#ifndef CLIENT_H_
#define CLIENT_H_

#include "macros.h"

int main(int argc, char *argv[]);

#if defined(__APPLE__) && defined(__MACH__)
void* server_listen(void* arg);
#else
int server_listen(void* arg);
#endif



#endif