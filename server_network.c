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
            printf("\nERRO: Invalid answer (cancel game with 'c') ");
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

            // only accept client_pkt's, get the queue lock in order to remove packets of other kinds from it
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

int serv_wait_players(int listen_sock, int *sock, struct sockaddr_in *addr, socklen_t *len) {
    int res = 0,
        n = 0,
        one = 1;
    char answ = 'a';
    int stdin_fd_flags = 0;
    struct pollfd pfds[6] = {0};

    // start polling just listen_sock and stdin
    pfds[0].fd = listen_sock;
    pfds[0].events = POLLIN;
    pfds[1].fd = fileno(stdin);
    pfds[1].events = POLLIN;
    for (int i = 2; i < 6; i++) {
        pfds[i].events = POLLIN;
        pfds[i].fd = -1;    
    }
    // the rest of the pfds are for checking the incoming player's connection status

    int newsock = -1;
    struct sockaddr_in newaddr = {0};
    socklen_t newlen = 0;

    // int i_dots = 0;
    // char str_dots[] = "...";
    // fd_set_nonblocking(fileno(stdin), &stdin_fd_flags); // should no longer be needed, we have poll
    printf("TRAC: Waiting for players...\n");
    bool returning = false;
    bool disconnection_free = true;
    while (true) {
        // innocent until proven guilty...
        disconnection_free = true;

        // FIXME: too hacky...
        if (returning == false && poll(pfds, n + 2, -1) == -1) {
            perror("poll");
            sleep(1);
            continue;
        }
        // if there is a connecting client, accept them and store their data in temporary variables
        if (pfds[0].revents & POLLIN && n < 4 && returning == false) {
            res = accept(listen_sock, (struct sockaddr *)&newaddr, &newlen);
            if (res == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    fprintf(stderr, "\n");
                    perror("WARN: accept");
                }
                sleep(1);
                continue;
            }
            newsock = res;
            
            res = setsockopt(res, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
            if (res == -1) {
                perror("ERRO: player setsockopt");
            }
            // res = fcntl(res, F_SETFL, O_NONBLOCK);
            // if (res == -1) {
            //     perror("ERRO: fcntl");
            // }
        }

        // fancy subroutine that removes the data of any client that might have disconnected in this phase
        // this must be executed befor placing the new client's socket and address data inside the data structures
        for (int i = 0; i < n; i++) {
            while (pfds[i+2].revents & POLLIN) {

                // reckless
                assert(pfds[i+2].fd != -1);

                int scrap_ret = 0;
                char scrap_buf[4];
                // NOTE: we will discard any data they might have sent us! (this is fine for now)
                scrap_ret = recv(pfds[i+2].fd, scrap_buf, 1, 0);
                if (scrap_ret == -1) 
                    perror("scrap recv:"); // we want to enter the next if block too
                if (scrap_ret != 0) { poll(&pfds[i+2], 1, 100); continue; }

                // if scrap_ret is 0, the i-th client has disconnected
                // remove them from pfds
                pfds[i+2].fd = -1;
                // overwrite their data with the data of the most recently connected client
                pfds[i+2].fd = pfds[n+1].fd;
                pfds[i+2].revents = pfds[n+1].revents;
                sock[i] = sock[n-1];
                addr[i] = addr[n-1];
                len[i] = len[n-1];
                // decrease i since we will need to verify the 'new' i-th client in the next iteration
                i--;
                // decrease n since we care about one less client now
                n--;

                // some I/O
                printf("\nINFO: A player disconnected, there %s now %d player%s waiting%s.\n", n == 1 ? "is" : "are",
                    n, n == 1 ? "" : "s", returning == true ? " (game start canceled)" : "");
                if (n >= 2) {
                    printf("QUST: Start the game anyways? [y/n]: ", n);
                    fflush(stdout);
                    flush_instream(stdin);
                }

                // reset exit flags (we shan't return just after a disconnection)
                disconnection_free = false;
                returning = false;

                // we no longer need the while loop (its just for recv-ing)
                break;

                // this routine should still work as expected even when i = n-1 
                // (in other words, when the most recently connected client is the one who got disconnected),
                // some redundancies were made in order to ensure a coherent state even in that case
            }
        }

        // return if we have stable clients and we've either reached the max of 4 or have gotten confirmation from stdin
        if (disconnection_free && returning) return n;

        // now we can add the new client
        if (newsock != -1) {
            pfds[n+2].fd = newsock;
            sock[n] = newsock;
            addr[n] = newaddr;
            len[n] = newlen;
            
            n++;

            // ensure that we enter this block only once per client
            newsock = -1; 

            printf("\n%s: A new player connected, ", n < 2 ? "INFO" : "QUST");
            if (n < 2) {
                printf("at least one more is needed to start.\n");
            } else {
                printf("start a game with %d players? [y/n]: ", n);
                fflush(stdout);
                flush_instream(stdin);
            }
        }

        sleep(1);

        // read stdin if there is something there and a question has been made (when n >= 2)
        if (pfds[1].revents & POLLIN && n >= 2) {
            answ = fgetc(stdin);
            switch (answ) {
            case 'Y':
            case 'y': 
                // should no longer be needed, we have poll
                // fd_unset_nonblocking(fileno(stdin), &stdin_fd_flags); 
                returning = true;
                break;
            default:
                printf("ERRO: Invalid answer.\n");
            case 'N':
            case 'n':
                printf("QUST: Start a game with %d players? [y/n]: ", n);
                fflush(stdout);
                flush_instream(stdin);
            case -1:
                continue;
            }
        }
    }
    printf("\nINFO: Starting a game with 4 players.\n");
    // fd_set_nonblocking(fileno(stdin), &stdin_fd_flags); // should no longer be needed, we have poll
    return 4;
}