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

// this function needs to get initialized once by calling it with a pointer to the Game_serv struct (will always return 0 from an initialization call);
// when called with NULL arguments, returns the number of disconnected players (0 if everything's fine), or -1 if called in this fashion without previous initialization.
int net_detect_disconnections(void *arg) {
    static struct Game_serv *g = NULL;
    if (arg != NULL) {
        g = arg;
        return 0;
    }
    if (g == NULL) return -1;

    g->disconnected_player_count = 0;
    for (int i = 0; i < g->player_count; i++) {
        if (g->players[i].netinfo.pk_queue.closed) {
            printf("ERRO: %d:%s disconnected.\n", g->players[i].id, g->players[i].name);
            g->disconnected_players[i] = &g->players[i];
            g->disconnected_player_count++;
        }
    }

    return g->disconnected_player_count;
}

// returns true if all disconnections were able to be resolved
bool net_handle_disconnections(struct Game_serv *g) {
    /*tmp*/ if (g == NULL) return true;
    if (g->disconnected_player_count == 0) return true;

    int res = 0;
    char answ = 'a';
    struct sockaddr_in d_addr = {0};
    socklen_t d_lens = 0;
    
    int stdin_fd_flags = 0;
    fd_set_nonblocking(fileno(stdin), &stdin_fd_flags);

    char print_nplayers_article[4] = { 0 };
    snprintf(print_nplayers_article, 2, "%d", g->disconnected_player_count);
    printf("INFO: Waiting for %s disconnected player%s (cancel game with 'c') ",
        g->disconnected_player_count == 1 ? "a" : print_nplayers_article,
        g->disconnected_player_count > 1 ? "s" : "");
    fflush(stdout);
    flush_instream(stdin);

    while (true) {
        res = accept(g->listen_sock, (struct sockaddr *)&d_addr, &d_lens);
        if (res != -1) {

            int found_id = -1;

            printf("\nINFO: A player reconnected, ");
            for (int n = 0; n < 4; n++) {
                if (g->disconnected_players[n] == NULL) continue;
                if (d_addr.sin_addr.s_addr == g->disconnected_players[n]->netinfo.addr.sin_addr.s_addr) {
                    found_id = g->disconnected_players[n]->id;
                    printf("they appear to be %d:%s\n", g->players[found_id].id, g->players[found_id].name);
                }
            }

            if (found_id == -1) {
                printf("they don't appear to have previously connected\n");
                // TODO
                // printf("QUST: Do you want to let them join as anyone? [y/n]: ");
                // fflush(stdout);
                // flush_instream(stdin);
                // TEMP:
                close(res); 
                printf("INFO: Waiting for %d disconnected player%s (cancel game with 'c') ",
                    g->disconnected_player_count,
                    g->disconnected_player_count > 1 ? "s" : "");
                fflush(stdout);
                flush_instream(stdin);
                continue;
            }

            g->disconnected_players[found_id] = NULL;
            g->disconnected_player_count--;

            thread_recv_init(&g->players[found_id].netinfo.pk_queue, res);
            net_notify_clients(
                g, (int[]) { g->players[found_id].id }, 
                1, RQ_NONE, EV_WELCOME, 
                &(struct EV_packet_welcome) { .id = g->players[found_id].id });

            if (g->disconnected_player_count <= 0) 
                break;

            printf("INFO: Waiting for %d disconnected player%s (cancel game with 'c') ", g->disconnected_player_count,
                g->disconnected_player_count > 1 ? "s" : "");
            fflush(stdout);
            flush_instream(stdin);
        }
        if (errno != EAGAIN || errno != EWOULDBLOCK) {
            fprintf(stderr, "\n");
            perror("WARN: accept");
            continue;
        }
        sleep(1);
        answ = fgetc(stdin);
        switch (answ) {
        case 'c':
            fd_unset_nonblocking(fileno(stdin), &stdin_fd_flags);
            return false;
        default:
            printf("\nERRO: Invalid answer (cancel game with 'c') \n");
        case -1:
            continue;
        }
    }
    fd_unset_nonblocking(fileno(stdin), &stdin_fd_flags);
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
    sp->status.pass_master_idx = g->pass_master_idx;
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

    {
        int n_disconnected = net_detect_disconnections(NULL);
        if (n_disconnected != 0) {
            if (net_handle_disconnections(g) == false) {
                return -1;
            }
        }
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
        if (net_detect_disconnections(NULL) != 0) {
            return NULL;
        }
        const size_t known_queue_size = pk_q->queue.size;
        struct PNode *pn = (struct PNode *)pk_q->queue.head;
        for (int i = 0; i < known_queue_size; pn = (struct PNode*)pn->node.next, i++) {
            assert(pn != NULL);

            if (net_detect_disconnections(NULL) != 0) {
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
