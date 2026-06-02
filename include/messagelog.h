#ifndef MSGLOG_H_
#define MSGLOG_H_

#include "macros.h"
#include "aes.h"
#define MAXLOG 30

struct log{
    int loglen;
    struct message* head;
    struct message* tail;
};

struct message{
    char msg[MESSAGE_LEN];
    char usr[USERNAME_LEN];

    uint8_t iv[AES_BLOCKLEN];

    struct message* next;
    struct message* last;
};

void init_log(struct log* log);

struct message* add_log(struct log* log, char* msg, char* usr, uint8_t* iv);

void free_log(struct log* log);

#endif