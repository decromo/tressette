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
#include <unistd.h>


#ifdef __MINGW32__
#include "windef.h"
#else
#include <sys/socket.h>
#endif // __MINGW32__

#include "common.h"
#include "network.h"
#include "threads.h"
#include "server.h"
#include "server_network.h"

bool handle_disconnections(void *arg) {
    /*tmp*/ if (arg == NULL) return true;

    struct Game_serv *g = arg;
    int n_disconnected = 0;
    for (int i = 0; i < g->player_count; i++) {
        if (g->players[i].netinfo.pk_queue.closed) {
            printf("ERRO: %d:%s disconnected.", g->players[i].id, g->players[i].name);
            g->disconnected_players[n_disconnected++] = &g->players[i];
        }
    }

    if (n_disconnected != 0) {
        int res = 0;

        char answ = 'a';

        struct sockaddr_in d_addrs = {0};
        socklen_t d_lens = 0;
        
        int stdin_fd_flags = 0;
        fd_set_nonblocking(fileno(stdin), &stdin_fd_flags);

        printf("INFO: Waiting for %d disconnected player%s (cancel game with 'c') ", n_disconnected,
            n_disconnected > 1 ? "s" : "");
        fflush(stdout);
        flush_instream(fileno(stdin));

        while (true) {
            res = accept(g->listen_sock, (struct sockaddr *)&d_addrs, &d_lens);
            if (res != -1) {

                int found_id = -1;

                printf("\nINFO: A player reconnected, ");
                for (int n = 0; n < 4; n++) {
                    if (g->disconnected_players[n] == NULL) continue;
                    if (d_addrs.sin_addr.s_addr == g->disconnected_players[n]->netinfo.addr.sin_addr.s_addr) {
                        found_id = g->disconnected_players[n]->id;
                        printf("they appear to be %d:%s\n", g->players[found_id].id, g->players[found_id].name);
                    }
                }

                if (found_id == -1) {
                    printf("they don't appear to have previously connected\n");
                    // TODO
                    // printf("QUST: Do you want to let them join as anyone? [y/n]: ");
                    // fflush(stdout);
                    // flush_instream(fileno(stdin));
                    // TEMP:
                    close(res); 
                    printf("INFO: Waiting for %d disconnected player%s (cancel game with 'c') ", n_disconnected,
                        n_disconnected > 1 ? "s" : "");
                    fflush(stdout);
                    flush_instream(fileno(stdin));
                    continue;
                }

                n_disconnected--;

                thread_recv_init(&g->players[found_id].netinfo.pk_queue, res);
                sleep(1); // this fixes the possible race condition :)
                net_notify_clients(
                    g, (int[]) { g->players[found_id].id }, 
                    1, RQ_NAME, EV_WELCOME, 
                    &(struct EV_packet_welcome) { .id = g->players[found_id].id });

                if (n_disconnected <= 0) 
                    break;

                printf("INFO: Waiting for %d disconnected player%s (cancel game with 'c') ", n_disconnected,
                    n_disconnected > 1 ? "s" : "");
                fflush(stdout);
                flush_instream(fileno(stdin));
            }
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                fprintf(stderr, "\n");
                perror("WARN: accept");
                continue;
            }
            sleep(2);
            answ = fgetc(stdin);
            switch (answ) {
            case 'c':
                fd_unset_nonblocking(fileno(stdin), &stdin_fd_flags);
                return false;
            default:
                printf("ERRO: Invalid answer, will not end game.\n");
            case -1:
                continue;
            }
        }
        fd_unset_nonblocking(fileno(stdin), &stdin_fd_flags);
    }
    return true;
}

struct Packet *net_serv_forge_packet(struct Game_serv *g, struct Player *p) {
    struct Packet *packet = calloc(1, PACKET_SIZE);
    assert(packet != NULL);
    packet->pk_kind = SERVER_PKT;
    struct Server_packet *sp = (struct Server_packet *)packet->data;
    sp->status.player_count = g->player_count;
    sp->status.is_team_game = g->team_game;
    sp->status.target_score = g->target_score;
    sp->status.round = g->round;
    sp->status.pass = g->pass;
    sp->status.pass_suit = g->pass_suit;
    sp->status.pass_master = g->pass_master;
    for (int i = 0; i < 4; i++) {
        if (g->pass_cards[i] == NULL) continue;
        sp->status.pass_cards[i].suit = g->pass_cards[i]->suit;
        sp->status.pass_cards[i].val = g->pass_cards[i]->value;
    }
    sp->status.turn_idx = g->turn_idx;
    sp->status.turn_counter = g->turn_counter;
    for (int i = 0; i < g->player_count; i++) {
        sp->status.game_scores[i] = g->players[i].game_score;
        sp->status.round_score_thirds[i] = g->players[i].round_score_thirds;
        strncpy(sp->status.names[i], g->players[i].name, PLAYERNAME_STRLEN);
    }
    sp->hand.round = g->round;
    sp->hand.n_cards = p->card_count;
    struct Card_node *cn = (struct Card_node *)p->hand.head;
    for (int i = 0; i < p->hand.size; i++) {
        sp->hand.cards[i] = (struct Packet_card){ .id = i, .suit = cn->c->suit, .val = cn->c->value };
        cn = (struct Card_node *)cn->node.next;
    }
    return packet;
}

int net_notify_clients(struct Game_serv *g, int whom[1], int n_whom,
                        enum Request_kind rq_kind, enum Event_kind ev_kind, void *ev_args) {
    int errors = 0, sock = 0;
    int whom_all[] = { 0, 1, 2, 3 };
    if (whom == NULL) {
        whom = whom_all;
        n_whom = g->player_count;
    }

    if (handle_disconnections(NULL) == false) {
        serv_shutdown_game(g);
        return 1;
    }

    struct Packet *packets[4] = { 0 };
    struct Server_packet *isp = NULL;
    for (int i = 0; i < n_whom; i++) {
        struct Player *p = &g->players[whom[i]];
        packets[i] = net_serv_forge_packet(g, p);
        isp = (struct Server_packet *)packets[i]->data;
        isp->rq_kind = rq_kind;
        isp->ev_kind = ev_kind;
        isp->ev_size = event_sizeof(ev_kind);
        if (ev_args != NULL) {
            memcpy(isp->ev_data, ev_args, isp->ev_size);
        }

        sock = p->netinfo.pk_queue.socket;
        printf("TRAC: Sending a ");
        if (rq_kind != RQ_NONE) {
            printf("/rq:%s\\", request_nameof(rq_kind));
        }
        if (ev_kind != EV_NONE) {
            printf("\\ev:%s/", event_nameof(ev_kind));
        }
        printf(" packet to %d:%s\n", p->id, p->name);

        errors += net_send_packet(sock, packets[i]);
    }

    for (int i = 0; i < n_whom; i++) {
        free(packets[i]);
    }
    return errors;
}

// TODO: timeout?
struct PNode *net_serv_need_response(enum Response_kind rs_k, struct PQueue *pk_q, int n, bool wait) {
    int n_found = 0;
    
    while (n_found <= n) {
        const size_t known_queue_size = pk_q->queue.size;
        struct PNode *pn = (struct PNode *)pk_q->queue.head;
        for (int i = 0; i < known_queue_size; pn = (struct PNode*)pn->node.next, i++) {
            assert(pn != NULL);

            if (handle_disconnections(NULL) == false) {
                return NULL;
            }

            // this is ugly but needed since pn will always point to the list head if this thread was blocked with an empty queue waiting for a first packet.
            if (pn == (struct PNode *)&pk_q->queue.head) {
                // if pn points to the head, we will restart the loop. this probably applies only if wait is true
                break;
            }

            assert(pn->pk != NULL);

            // only accept client_pkt's, remove the others from queue by acquiring its lock
            if (pn->pk->pk_kind != CLIENT_PKT) {
                free(pn->pk);
                llist_mtsafe(pk_q, remove, pn);
                continue;
            }

            struct Client_packet *cp = (struct Client_packet *)pn->pk->data;
            // only return the n-th (0 index) packet with the correct response
            // FIXME: this is clunky and unflexble, try to handle it another way
            if (cp->rs_kind == rs_k && n_found++ == n) {
                return pn;
            }
        }

        // packet wasn't available when the function was called, 
        // return failure if we're not wating
        if (wait == false) {
            return NULL;
        }

        usleep(300);
    }

    return NULL;
}

void net_get_playername(struct Player *p, int maxsize) {
    struct PNode *pn = NULL;
    struct Client_packet *cp = NULL;
    struct RS_packet_name *rsp = NULL;

    pn = net_serv_need_response(RS_NAME, &p->netinfo.pk_queue, 0, true);
    assert(pn != NULL && pn->pk != NULL);
    cp = (struct Client_packet *)pn->pk->data;
    assert(cp->rs_kind == RS_NAME);
    rsp = (struct RS_packet_name *)cp->rs_data;

    if (rsp->name[0] == '\0' || rsp->name[0] == '\n') {
        // got an empty name
        printf("INFO: Player %d didn't give a name, giving them a default one\n", p->id);
        snprintf(p->name, maxsize - 1, "Player %d", p->id + 1);
    } else {
        for (int i = 0; i < maxsize; i++) {
            if (rsp->name[i] == '\0' || rsp->name[i] == '\n') {
                p->name[i] = '\0';
                break;
            }
            if (rsp->name[i] >= 32 && rsp->name[i] <= 126)
                p->name[i] = rsp->name[i];
            else
                p->name[i] = '.';
        }
    }
    p->name[maxsize - 1] = '\0';
    printf("TRAC: Player %d's name is %s\n", p->id, p->name);

    free(pn->pk);
    llist_mtsafe(&p->netinfo.pk_queue, remove, pn);
    return;
}

int net_get_playermove(struct Player *p, int of_round, int of_pass) {
    int card_id = -1;
    int deltaround, deltapass;
    struct PNode *pn = NULL;
    struct Client_packet *cp = NULL;
    struct RS_packet_move *rsp = NULL;

    for (int i = 0; i < LOOKFOR_MAXPACKETS; i++) {
        pn = net_serv_need_response(RS_MOVE, &p->netinfo.pk_queue, i, true);
        assert(pn != NULL);
        cp = (struct Client_packet *) pn->pk->data;
        assert(cp->rs_kind == RS_MOVE);
        rsp = (struct RS_packet_move *) cp->rs_data;

        deltaround = rsp->round - of_round;
        deltapass = rsp->pass - of_pass;
        if (deltaround < 0 || (deltaround == 0 && deltapass < 0)) {
            printf("INFO: Found an old RS_MOVE packet (from %d:%s on %d:%d)", p->id, p->name, rsp->round + 1, rsp->pass + 1);
            free(pn->pk); // FIXME: can I really free the packet without acquiring the lock? consuming
            llist_mtsafe(&p->netinfo.pk_queue, remove, pn);
            i--;
            continue;
        }
        if ((deltaround == 0) && (deltapass == 0)) {
            card_id = rsp->card_id;
            printf("INFO: Got a RS_MOVE packet from %d:%s (card_id %d)\n", p->id, p->name, card_id);
            free(pn->pk); // FIXME: can I really free the packet without acquiring the lock? consuming
            llist_mtsafe(&p->netinfo.pk_queue, remove, pn);
            return card_id;
        }
    }
    return -1;
}