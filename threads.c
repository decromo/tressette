#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>

#include "network.h"
#include "common.h"
#include "threads.h"

// read more: man 7 pthreads ; man pthread_cancel


// TODO: cleanup thread, mutex, attr, and socket maybe?
void *thread_recv_main(void *arg) {
    struct PQueue *pk_q = arg;
    struct pollfd pollfd = { .fd = pk_q->socket, .events = POLLIN, .revents = 0 };

    pthread_cleanup_push(thread_recv_destroy, pk_q);

    while (true) {
        switch (poll(&pollfd, 1, -1)) {
        case 0: sleep(1); fprintf(stderr, "INFO: poll() returned 0\n"); continue;
        case 1: break;
        default: perror("poll"); assert(false);
        }
        if (pollfd.revents != 0) {
            if (pollfd.revents & POLLERR) {
                // something went wrong, cleanup and exit
                printf("\nTRAC: got POLLERR\n");
                pthread_exit(NULL);
            }
            if (pollfd.revents & POLLNVAL) {
                // invalid socket, cleanup and exit
                printf("\nTRAC: got POLLNVAL\n");
                pthread_exit(NULL);
            }
            if (pollfd.revents & POLLHUP) {
                // another thread closed the socket, cleanup and exit
                printf("\nTRAC: got POLLHUP\n");
                pthread_exit(NULL);
            }
            if (pollfd.revents & POLLIN) {
                struct Packet *packet = net_recv_packet(pollfd.fd);
                // check if peer closed connection
                if (packet == NULL) {
                    pk_q->closed = true;
                    pthread_exit(NULL);
                }
                struct PNode *pn = calloc(1, sizeof(struct PNode));
                assert(pn != NULL);
                pn->pk = packet;
                pthread_mutex_lock(&pk_q->lock);
                llist_append(&pk_q->queue, &pn->node);
                pthread_mutex_unlock(&pk_q->lock);
            }
        } else {
            sleep(1); continue;
        }
    }
    pthread_cleanup_pop(0);
}


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

    pk_q->closed = false;

    res = pthread_create(&pk_q->pt_id, &pt_attr, thread_recv_main, pk_q);
    assert(res == 0);

    res = pthread_attr_destroy(&pt_attr);
    assert(res == 0);
    return pk_q->pt_id;
}

void thread_recv_destroy(void *arg) {
    printf("\nTRAC: destroying thread %ld\n", pthread_self());

    struct PQueue *pk_q = arg;

    pthread_mutex_lock(&pk_q->lock);

    llist_nuke(&pk_q->queue, free_pnode_packet);

    pthread_mutex_destroy(&pk_q->lock);
    close(pk_q->socket);
}

struct Packet *pop_packet(struct PQueue *pq) {
    if (pq->queue.size == 0 || pq->queue.head == (struct llist_node*) &pq->queue)
        return NULL;

    struct Packet *ret = ((struct PNode*) pq->queue.head)->pk;
    llist_mtsafe(pq, remove, pq->queue.head);

    return ret;
}