#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "network.h"
#include "threads.h"

// read more: man 7 pthreads ; man pthread_cancel

// this initializes an empty PQueue for a new player and starts the relative thread
pthread_t thread_recv_init(struct PQueue *pk_q, int sockfd) {
    int res = 0;

    pthread_attr_t pt_attr;
    res = pthread_attr_init(&pt_attr);
    res += pthread_attr_setdetachstate(&pt_attr, PTHREAD_CREATE_DETACHED);
    assert(res == 0);

    llist_init(&pk_q->queue);

    pk_q->lock = (pthread_mutex_t)PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

    pk_q->socket = sockfd;

    res = pthread_create(&pk_q->pt_id, &pt_attr, thread_recv_main, pk_q);
    assert(res == 0);

    res = pthread_attr_destroy(&pt_attr);
    assert(res == 0);
    return pk_q->pt_id;
}

struct Packet *pop_packet(struct PQueue *pq) {
    if (pq->queue.size == 0 || pq->queue.head == (struct llist_node*) &pq->queue)
        return NULL;

    struct Packet *ret = ((struct PNode*) pq->queue.head)->pk;
    llist_mtsafe(pq, remove, pq->queue.head);

    return ret;
}