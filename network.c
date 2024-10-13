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

struct Packet *net_recv_packet(int sock) {
    const int recv_size = 32;
    int recived = 0,
        res = 0;
    char buf[PACKET_SIZE];
    while (recived != PACKET_SIZE) {
        res = recv(sock, &buf[recived], recv_size, 0);
        switch (res) {
        // case -1:
        //     switch (errno) {
        //         case EAGAIN:
        //             sleep(1);
        //             continue;
        //         default:
        //             break;
        //     }
        //     perror("recv");
        //     return NULL;
        case 0:
            // connection was closed by peer
            return NULL;
        default:
            recived += res;
        }
    }

    struct Packet *pbuf = calloc(1, PACKET_SIZE);
    assert(pbuf != NULL);
    char *ptr = (char *)pbuf;
    for (int i = 0; i < PACKET_SIZE; i++) {
        ptr[i] = buf[PACKET_SIZE - 1 - i];
    }

    return pbuf;
}

int net_send_packet(int sock, struct Packet *packet) {
    char buf[PACKET_SIZE];
    const int data_bytes = PACKET_SIZE - 1;

    // set the non-data bytes
    buf[data_bytes] = packet->pk_kind;
    char *data = (char *)packet->data;

    // set the data bytes
    for (int i = 1; i <= data_bytes; i++) {
        buf[data_bytes - i] = data[i - 1];
    }
    int sent = 0,
        res = 0;
    while (sent != PACKET_SIZE) {
        res = send(sock, &buf[sent], PACKET_SIZE - sent, 0);
        if (res == -1) {
            if (errno != EBADF) perror("send");
            return -1;
        }
        sent += res;
    }
    return 0;
}

void free_pnode_packet(void *arg) {
    struct PNode *pn = arg;
    free(pn->pk);
}