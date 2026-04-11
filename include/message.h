#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <time.h>

struct Message{
    char user[16];
    char message[256];
    time_t sent;
};

#endif // MESSAGE_H_