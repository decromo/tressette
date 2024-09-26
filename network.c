#include <stdio.h>
#include <stdlib.h>
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
            case -1:
                switch (errno) {
                    case EAGAIN:
                        sleep(1);
                        continue;
                    default:
                        break;
                }
                perror("recv");
                return NULL;
            case 0:
                // TODO: handle closed connections;
                fprintf(stderr, "ERRO: Connection on socket %d closed.\n", sock);
                exit(1);
            default:
                break;
        }
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
// int net_send_packet(int sock, struct Packet *packet) {
//     int sent = 0,
//         res = 0;
//     while (sent != sizeof(struct Packet)) {
//         res = send(sock, &packet, sizeof(struct Packet) - sent, 0);
//         if (res == -1) {
//             perror("ERRO: send, cards");
//             return -1;
//         }
//         sent += res;
//         res = 0;
//     }
//     return 0;
// }
int net_send_packet(int sock, struct Packet *packet) {
    char buf[PACKET_SIZE];
    buf[PACKET_SIZE - 1] = packet->kind;
    buf[PACKET_SIZE - 2] = packet->size;
    char *data = (char*)packet->data;
    for (int i = 0; i <= PACKET_SIZE - 3; i++) {
        buf[PACKET_SIZE - 3 - i] = data[i];
    }
    int sent = 0,
        res = 0;
    while (sent != PACKET_SIZE) {
        res = send(sock, &buf[sent], PACKET_SIZE - sent, 0);
        if (res == -1) {
            perror("ERRO: send, cards");
            return -1;
        }
        sent += res;
        res = 0;
    }
    return 0;
}
struct PNode *net_need_packetkind(enum Packet_kind kind, llist *q, int n, bool wait, int sock) {
    int n_found = 0;
    struct PNode *pn = (struct PNode*) q->head;
    while (true) {  // TODO: timeout?
        for (int i = 0; i < q->size;  i++) {
            assert(pn != NULL && pn->pk != NULL);
            if (pn->pk->kind == NAME && n_found == n) {
                return pn;
            }
            pn = (struct PNode*) pn->node.next;
        }
        if (wait == true) {
            pn = calloc(1, sizeof (*pn));
            assert(pn != NULL);
            pn->pk = net_recv_packet(sock);
            if (pn->pk == NULL) {
                fprintf(stderr, "WARN: Got hold of a NULL packet, ignoring\n");
                free(pn);
                continue;
            }
            llist_append(q, (llist_node*) pn);
            if (pn->pk->kind == kind) {
                return pn;
            }
        }
    }
}