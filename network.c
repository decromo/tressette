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

// TODO: add a bool function that returns if a certain packet kind (w/ request/response/event) is present in the queue
// i want to use it in a client subroutine which will prompt the client user for who they want to reconnect as:
// it will be useful in order to refresh the prompt if any more reconnections or disconnections happen while choosing.

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

// returns true if a packet of a particular type is present in the given queue.
// note that the needed parameters depend on pk_k
bool net_query_queue(struct PQueue *pk_q, enum Packet_kind pk_k, 
    enum Request_kind rq_k, enum Event_kind ev_k, enum Response_kind rs_k) {
    
    int known_size = pk_q->queue.size;
    struct PNode *pn = pk_q->queue.head;
    struct Server_packet *spk = NULL;
    struct Client_packet *cpk = NULL;

    for (int i = 0; i < known_size; i++) {
        // filter out for packet kind
        if (pn->pk->pk_kind != pk_k) continue;

        switch (pn->pk->pk_kind) {
        case SERVER_PKT:
            spk = (struct Server_packet *)pn->pk->data;
            if (spk->rq_kind == rq_k && spk->ev_kind == ev_k)
                return true;    // found a matching packet
            break;
        case CLIENT_PKT:
            cpk = (struct Client_packet *)pn->pk->data;
            if (cpk->rs_kind == rq_k)
                return true;    // found a matching packet
            break;
        }
    }
    // didn't found that type of packet in queue
    return false;
}