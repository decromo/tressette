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