#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <time.h>

#define MESSAGE_LEN 265
#define USERNAME_LEN 16

struct Message{
    char user[USERNAME_LEN];
    char text[MESSAGE_LEN];
    time_t sent;
};

#endif // MESSAGE_H_