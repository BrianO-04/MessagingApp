#ifndef CLIENT_H_
#define CLIENT_H_

#include "macros.h"
#include "messagelog.h"


THRDFUNC server_listen(void* arg);

struct log* client_init(char* uname, char* ip, int* new_msg, mtx_t* logLock);

void send_code(cmd_types cmd);
void send_msg(char msg[MESSAGE_LEN]);

#endif