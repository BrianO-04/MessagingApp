#ifndef MSGLOG_H_
#define MSGLOG_H_

#include "macros.h"
#define MAXLOG 30

struct log{
    int loglen;
    struct message* head;
    struct message* tail;
};

struct message{
    char msg[MESSAGE_LEN+USERNAME_LEN];

    struct message* next;
    struct message* last;
};

void init_log(struct log* log);

struct message* add_log(struct log* log, char* msg);

#endif