#ifndef MACROS_H_
#define MACROS_H_

#include <time.h>

// Windows Sockets
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <BaseTsd.h>
#else // Posix sockets
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// MacOS does not support threads.h so use pthread.h instead
#if defined(__APPLE__) && defined(__MACH__)
#include <pthread.h>
#else
#include <threads.h>
#endif

#define MESSAGE_LEN 256
#define USERNAME_LEN 16
#define PORT 8080
#define MAXLOG 30

typedef enum{
    EMPTY,
    MESSAGE,
    JOIN,
    LIST
} cmd_types;

// Multiplatform socket reading
#if defined(_WIN32)
int read_mp(SOCKET fd, void* buf, int nbytes);
#else
int read_mp(int fd, void* buf, size_t nbytes);
#endif

// Map SOCKET types from windows to integer for POSIX
#if !defined(_WIN32)
#define SOCKET int
#define closesocket close
#endif

// Map POSIX threads to C threads
#if defined(__APPLE__) && defined(__MACH__)
#define THRDFUNC void*
#define THRDEXIT NULL
#define thrd_exit pthread_exit

#define thrd_t pthread_t

int thrd_create(thrd_t* thr, void *(*start_routine)(void*), void *arg);

// Not Mac
#else
#define THRDFUNC int
#define THRDEXIT 0
#endif

#endif // MACROS_H_