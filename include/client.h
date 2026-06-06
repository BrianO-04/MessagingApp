#ifndef CLIENT_H_
#define CLIENT_H_

#include "macros.h"
#include "messagelog.h"


THRDFUNC server_listen(void* arg);

int client_init(char* uname, char* ip);



#endif