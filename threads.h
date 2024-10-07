#ifndef THREADS_H
#define THREADS_H

#include <stdint.h>
#include "common.h"

typedef uint8_t u8;

#define llist_mtsafe(pqueue_ptr, operation, ...) do { \
        pthread_mutex_lock(&((pqueue_ptr)->lock)); \
        llist_##operation##(&((pqueue_ptr)->queue), __VA_ARGS__); \
        pthread_mutex_unlock(&((pqueue_ptr)->lock)); \
    } while (0);

pthread_t thread_recv_init(struct PQueue *pk_q, int sockfd);

#endif // THREADS_H

