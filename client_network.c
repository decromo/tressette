#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>
#include <string.h>
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
#include "threads.h"
#include "client.h"
#include "client_network.h"

// struct PNode *net_serv_need_response(enum Response_kind rs_k, struct PQueue *pk_q, int n, bool wait) {
//     int n_found = 0;
//     struct PNode *pn = (struct PNode *)pk_q->queue.head;
//     while (n_found <= n) {  // TODO: timeout?
//         const size_t known_queue_size = pk_q->queue.size;
//         for (int i = 0; i < known_queue_size; pn = (struct PNode*)pn->node.next, i++) {
//             assert(pn != NULL && pn->pk != NULL);

//             // only accept client_pkt's, remove the others from queue by acquiring its lock
//             if (pn->pk->pk_kind != CLIENT_PKT) {
//                 free(pn->pk);
//                 llist_mtsafe(pk_q, remove, pn);
//                 continue;
//             }

//             struct Client_packet *cp = (struct Client_packet *)pn->pk->data;
//             // only return the n-th (0 index) packet with the correct response
//             // FIXME: this is clunky and unflexble, try to handle it another way
//             if (cp->rs_kind == rs_k && n_found++ == n) {
//                 return pn;
//             }
//         }

//         // packet wasn't available when the function was called, 
//         // return failure if we're not wating
//         if (wait == false) {
//             return NULL;
//         }

//         usleep(300);
//     }
// }

int net_contact_server(struct Game_client *g, enum Response_kind rs_k, void *rs_args) {
    int errors = 0;
    
    struct Packet packet = {0};
    packet.pk_kind = CLIENT_PKT;
    struct Client_packet *cp = (struct Client_packet *)&packet.data;

    cp->rs_kind = rs_k;
    cp->rs_size = response_sizeof(rs_k);
    if (rs_args != NULL) {
        memcpy(cp->rs_data, rs_args, cp->rs_size);
    }
    
    errors += net_send_packet(g->player.netinfo.pk_queue.socket, &packet);
    
    return errors;
}