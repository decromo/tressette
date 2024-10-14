#ifndef WINDEF_H
#define WINDEF_H


#ifdef __MINGW32__
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
static u_long winsock_one = 1;
// #include <ws2def.h>
// #include <iphlpapi.h>
// #include <windows.h>
// #include <io.h>
#define close closesocket
#define exit(exit_arg) \
    WSACleanup(); \
    exit(exit_arg);
#define accept(sock, addr, len) \
    accept(sock, (struct sockaddr*)addr, len)
#define setsockopt(sock, level, optname, optval, optlen) \
    setsockopt(sock, level, optname, "1", 4)
#define fcntl(sock, op, flag) ioctlsocket(sock, FIONBIO, &winsock_one)
#define shutdown(res, whom) 0
#define drand48() ((double)rand())/RAND_MAX
#define srand48(seed) srand(seed)

#define _NTDDI_VERSION_FROM_WIN32_WINNT2(ver)    ver##0000
#define _NTDDI_VERSION_FROM_WIN32_WINNT(ver)     _NTDDI_VERSION_FROM_WIN32_WINNT2(ver)
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x501
#endif // _WIN32_WINNT
#ifndef NTDDI_VERSION
#  define NTDDI_VERSION _NTDDI_VERSION_FROM_WIN32_WINNT(_WIN32_WINNT)
#endif // NTDDI_VERSION

static inline ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp) {
    char *ptr, *eptr;
    if (*buf == NULL || *bufsiz == 0) {
            *bufsiz = BUFSIZ;
            if ((*buf = malloc(*bufsiz)) == NULL)
                    return -1;
    }
    for (ptr = *buf, eptr = *buf + *bufsiz;;) {
            int c = fgetc(fp);
            if (c == -1) {
                    if (feof(fp)) {
                            ssize_t diff = (ssize_t)(ptr - *buf);
                            if (diff != 0) {
                                    *ptr = '\0';
                                    return diff;
                            }
                    }
                    return -1;
            }
            *ptr++ = c;
            if (c == delimiter) {
                    *ptr = '\0';
                    return ptr - *buf;
            }
            if (ptr + 2 >= eptr) {
                    char *nbuf;
                    size_t nbufsiz = *bufsiz * 2;
                    ssize_t d = ptr - *buf;
                    if ((nbuf = realloc(*buf, nbufsiz)) == NULL)
                            return -1;
                    *buf = nbuf;
                    *bufsiz = nbufsiz;
                    eptr = nbuf + nbufsiz;
                    ptr = nbuf + d;
            }
    }
}
#define getline(buf, bufsiz, fp) \
    getdelim(buf, bufsiz, '\n', fp);

#endif // __MINGW_32__

#endif // WINDEF_H