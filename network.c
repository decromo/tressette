#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>


#ifdef __MINGW32__
    #include "windef.h"
#else
    #include <sys/socket.h>
#endif // __MINGW32__

#include "common.h"
#include "network.h"

// struct Packet *net_recv_packet(int sock) {
//     const int recv_size = 32;
//     int recived = 0,
//         res = 0;
//     char buf[PACKET_SIZE];
//     while (recived != PACKET_SIZE) {
//         res = recv(sock, &buf[recived], recv_size, 0);
//         switch (res) {
//             case -1:
//                 switch (errno) {
//                     case EAGAIN:
//                         sleep(1);
//                         continue;
//                     default:
//                         break;
//                 }
//                 perror("recv");
//                 return NULL;
//             case 0:
//                 // TODO: handle closed connections;
//                 fprintf(stderr, "ERRO: Connection on socket %d closed.\n", sock);
//                 exit(1);
//             default:
//                 break;
//         }
//         recived += res;
//         res = 0;
//     }

//     struct Packet *pbuf = calloc(1, PACKET_SIZE);
//     assert(pbuf != NULL);
//     char *ptr = (char*)pbuf;
//     for (int i = 0; i < PACKET_SIZE; i++) {
//         ptr[i] = buf[PACKET_SIZE-1 - i];
//     }

//     return pbuf;
//}

// TODO: cleanup thread, mutex, attr, and socket maybe?
void thread_recv_main(void *pk_q_void) {
    struct PQueue *pk_q = pk_q_void;
    struct pollfd pollfd = { .fd = pk_q->socket, .events = POLLIN, .revents = 0};
    
    while (true) {
        switch(poll(&pollfd, 1, -1)) {
            case 0: sleep(1); fprintf(stderr, "INFO: poll() returned 0\n"); continue;
            case 1: break;
            default: perror("poll"); assert(false);
        }
        if (pollfd.revents != 0) {
            if (pollfd.revents & POLLIN) {
                struct Packet *packet = net_recv_packet(pollfd.fd);
                struct PNode *pn = calloc(1, sizeof(struct PNode));
                assert(pn != NULL);
                pn->pk = packet;
                pthread_mutex_lock(&pk_q->lock);
                llist_append(&pk_q->queue, &pn->node);
                pthread_mutex_unlock(&pk_q->lock);
            }
            else {
                if (pollfd.revents & POLLHUP) {
                    // finish reading and let main know peer closed stream
                }
                if (pollfd.revents & POLLNVAL) {
                    // socket is closed, cleanup and exit
                }
                if (pollfd.revents & POLLERR) {
                    // die?
                }
            }
        } else {
            sleep(1); continue;
        }
    }
}
struct Packet *net_recv_packet(int sock) {

    const int recv_size = 32;
    int recived = 0,
        res = 0;
    char buf[PACKET_SIZE];
    while (recived != PACKET_SIZE) {
        res = recv(sock, &buf[recived], recv_size, 0);
        // switch (res) {
        //     case -1:
        //         switch (errno) {
        //             case EAGAIN:
        //                 sleep(1);
        //                 continue;
        //             default:
        //                 break;
        //         }
        //         perror("recv");
        //         return NULL;
        //     case 0:
        //         // TODO: handle closed connections;
        //         fprintf(stderr, "ERRO: Connection on socket %d closed.\n", sock);
        //         exit(1);
        //     default:
        //         break;
        // }
        recived += res;
        res = 0;
    }

    struct Packet *pbuf = calloc(1, PACKET_SIZE);
    assert(pbuf != NULL);
    char *ptr = (char*)pbuf;
    for (int i = 0; i < PACKET_SIZE; i++) {
        ptr[i] = buf[PACKET_SIZE-1 - i];
    }

    return pbuf;
}

int net_send_packet(int sock, struct Packet *packet) {
    char buf[PACKET_SIZE];
    const int data_bytes = PACKET_SIZE - 1;

    // set the non-data bytes
    buf[data_bytes] = packet->pk_kind;
    char *data = (char*)packet->data;

    // set the data bytes
    for (int i = 1; i <= data_bytes; i++) {
        buf[data_bytes - i] = data[i-1];
    }
    int sent = 0,
        res = 0;
    while (sent != PACKET_SIZE) {
        res = send(sock, &buf[sent], PACKET_SIZE - sent, 0);
        if (res == -1) {
            perror("send");
            return -1;
        }
        sent += res;
        res = 0;
    }
    return 0;
}