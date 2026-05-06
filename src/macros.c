#include "macros.h"



#if defined(_WIN32)
int read_mp(SOCKET fd, void* buf, int nbytes){
    return recv(fd, buf, nbytes, 0);
}
#else
int read_mp(int fd, void* buf, size_t nbytes){
    return read(fd, buf, nbytes);
}
#endif

// Map POSIX threads to C threads
#if defined(__APPLE__) && defined(__MACH__)

int thrd_create(thrd_t* thr, void *(*start_routine)(void*), void *arg){
    return pthread_create(thr, NULL, start_routine, arg);
}

#endif