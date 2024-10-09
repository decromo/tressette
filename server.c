
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

#ifdef __MINGW32__
#   include "windef.h"
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#endif // __MINGW32__

#include "server.h"
#include "server_network.h"

#include "common.h"
#include "network.h"
#include "threads.h"

// int admin_idx = 0;

// TODO: look into accept4()

void give_cards(struct Player ps[2], int n_pl, struct Card *deck) {
    int shuffled_order[40];
    for (int i = 0; i < 40; i++) {
        shuffled_order[i] = i;
    }
    for (int i = 39; i > 0; i--) {
        if (n_pl == 3 && deck[shuffled_order[i]].suit == SPADE && deck[shuffled_order[i]].value == 5) {
            int k = shuffled_order[39];
            shuffled_order[i] = k;
            shuffled_order[39] = i;
        }
        int roll = (unsigned int)(drand48() * (i));
        int t = shuffled_order[roll];
        shuffled_order[roll] = shuffled_order[i];
        shuffled_order[i] = t;
    }

    int idx = 0;
    const int cards_pp = 40 / n_pl;
    for (int i = 0; i < n_pl; i++) {
        ps[i].round_score_thirds = 0;
        ps[i].card_count = cards_pp;
        llist_nuke(&ps[i].hand, NULL);
        struct Card_node *cnp;
        for (int j = 0; j < cards_pp; j++) {
            cnp = malloc(sizeof(struct Card_node));
            assert(cnp != NULL);
            cnp->c = &deck[shuffled_order[idx]];

            llist_add((llist *)&(ps[i].hand), (llist_node *)cnp);
            idx++;
        }
    }
}
// TODO: maybe break this function up?
void serv_simulate_round(struct Game_serv *g) {
    int cards_remaining = (40 / g->player_count) * g->player_count;

    g->pass_master = g->round % g->player_count,
        g->pass = 0;

    int thrown_totval = -1, extra_point = -1,
        pass_winner_id = -1;

    struct Player *turn_player = NULL;
    int turn_card_id = -1;
    struct Card_node *turn_card = NULL;

    net_notify_clients(g, NULL, 0, RQ_NONE, EV_ROUND_START, NULL);

    while (cards_remaining > 0) {
        // pass start
        g->pass_suit = -1;
        memset(g->pass_cards, 0, g->player_count * sizeof(struct Card *));
        g->turn_counter = 0;

        // last pass is worth 1 more full point
        extra_point = 3 * (cards_remaining == g->player_count);

        printf("TRAC: Round %d, Pass %d\n", g->round + 1, g->pass + 1);

        net_notify_clients(g, NULL, 0, RQ_NONE, EV_PASS_START, NULL);

        do {
            // turn start
            turn_card = NULL;
            g->turn_idx = (g->pass_master + g->turn_counter) % g->player_count;
            turn_player = &g->players[g->turn_idx];

            net_notify_clients(g, NULL, 0, RQ_NONE, 
                EV_TURN_START, &(struct EV_packet_turnstart) {.who = g->turn_idx});

            bool response_invalid = false;
            do {
                net_notify_clients(g, (int[]) { g->turn_idx }, 1,
                    response_invalid ? RQ_MOVE_INVALID : RQ_MOVE, EV_NONE, NULL);
                turn_card_id = net_get_playermove(turn_player, g->round, g->pass);
                assert(turn_card_id != -1);
                turn_card = find_valid_card((struct Card_node *)turn_player->hand.head,
                    turn_player->card_count, turn_card_id, g->pass_suit);
                response_invalid = true;  // setting this true so we can make a different request if we loop
            } while (turn_card == NULL);
            printf("NOTE: Played card is %d of %s:%d\n", turn_card->c->value + 1, suit_to_string(turn_card->c->suit), turn_card->c->value);

            // set the first card thrown as the main suit
            if (g->turn_counter == 0) {
                g->pass_suit = turn_card->c->suit;
            };

            // record played card
            g->pass_cards[g->turn_idx] = turn_card->c;

            // let players know about the played card
            {
                struct Packet_card pc = { .suit = turn_card->c->suit, .val = turn_card->c->value };
                net_notify_clients(g, NULL, 0, RQ_NONE, EV_PLAYED_CARD,
                                   &(struct EV_packet_playedcard){.whose = g->turn_idx, .card = pc});
            }

            // remove played card from player's hand
            llist_remove(&turn_player->hand, turn_card);
            turn_player->card_count -= 1;
        } while (++g->turn_counter < g->player_count);

        // all turns over, calculate pass reward
        thrown_totval = calculate_pass_value(g->pass_cards, g->player_count) + extra_point;
        pass_winner_id = select_pass_winner(g->pass_cards, g->player_count, g->pass_master);
        g->players[pass_winner_id].round_score_thirds += thrown_totval;

        // print pass reward
        printf("TRAC: %s:%d won %d", g->players[pass_winner_id].name, pass_winner_id + 1, thrown_totval / 3);
        if (thrown_totval % 3)
            printf(".%d", (thrown_totval % 3) * 33);
        printf(" points with %d of %s --\n", g->pass_cards[pass_winner_id]->value + 1,
               suit_to_string(g->pass_cards[pass_winner_id]->suit));

        // let players know somebody won
        net_notify_clients(g, NULL, 0, RQ_NONE, EV_PASS_OVER,
                           &(struct EV_packet_passover){.point_thirds_won = thrown_totval, .winner_id = pass_winner_id});

        // prepare for next pass
        g->pass_master = pass_winner_id;
        cards_remaining -= g->player_count;
        g->pass++;
    }

    // all passes over
    int score_deltas[4] = { 0 };

    for (int i = 0; i < g->player_count; i++) {
        score_deltas[i] = (int)(g->players[i].round_score_thirds / 3);
        g->players[i].game_score = score_deltas[i];
    }
    // round over
    {
        struct EV_packet_roundover pr = { .round = g->round };
        for (int i = 0; i < g->player_count; i++) { pr.score_deltas[i] = score_deltas[i]; };
        net_notify_clients(g, NULL, 0, RQ_NONE, EV_ROUND_OVER, &pr);
    }
}
int select_pass_winner(struct Card **thrown, int n_thrown, int pass_master_idx) {
    assert(pass_master_idx < n_thrown);
    enum Suits main_suit = thrown[pass_master_idx]->suit;
    int winning_player = pass_master_idx;
    int winning_strenght = thrown[pass_master_idx]->value;
    printf("TRAC: Main Suit is %s, from Player %d.", suit_to_string(main_suit), pass_master_idx);
    int card_strenght;
    for (int i = 0; i < n_thrown; i++) {
        printf("TRAC: %d played %d of %s", i, thrown[i]->value + 1, suit_to_string(thrown[i]->suit));

        card_strenght = thrown[i]->value;
        // 0, 1 and 2 (ace, two and three) will have a strenght of 10, 11 and 12 respectively
        if (card_strenght <= 2) {
            card_strenght += 10;
            printf(" upgraded to %d", card_strenght);
        }

        if (thrown[i]->suit == main_suit && card_strenght > winning_strenght) {
            winning_strenght = card_strenght;
            winning_player = i;
        }
        printf("\n");
    }
    printf("\n");
    return winning_player;
}
bool is_game_over_serv(struct Player ps[1], int n_players, int target) {
    for (int i = 0; i < n_players; i++)
        if (ps[i].game_score >= target)
            return true;
    return false;
}

int serv_listen(char *port) {
    int sockfd = -1, res, one = 1;
    struct addrinfo hints, *ai_res, *ain;
    char default_port[8];
    if (port == NULL) {
        snprintf(default_port, 5, "%d", PORT_DEFAULT);
        port = default_port;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if ((res = getaddrinfo(NULL, default_port, &hints, &ai_res)) != 0) {
        fprintf(stderr, "FATL: gai = %s\n", gai_strerror(res));
        return -1;
    }

    for (ain = ai_res; ain != NULL; ain = ain->ai_next) {
        if ((res = socket(ain->ai_family, ain->ai_socktype, ain->ai_protocol)) == -1) {
            perror("WARN: socket");
            continue;
        }
        sockfd = res;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) == -1) {
            perror("ERRO: setsockopt");
        }

        if (bind(sockfd, ain->ai_addr, ain->ai_addrlen) == 0) {
            if (listen(sockfd, 5) == 0) {
                if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
                    perror("ERRO: fcntl");
                }
                break;
            }
        }
        if (shutdown(sockfd, SHUT_RDWR) == -1) {
            perror("WARN: shutdown");
        }
        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(ai_res);

    if (ain == NULL) {
        fprintf(stderr, "FATL: Could not listen on any address...\n");
        return -1;
    }

    return sockfd;
}
int serv_wait_players(int listen_sock, int *sock, struct sockaddr_in *addr, socklen_t *len) {
    int res = 0,
        n = 0,
        one = 1;
    char answ = 'a';
    int stdin_fd_flags = 0;
    // int i_dots = 0;
    // char str_dots[] = "...";
    printf("TRAC: Waiting for players...\n");
    fd_set_nonblocking(fileno(stdin), &stdin_fd_flags);
    while (n < 4) {
        res = accept(listen_sock, &addr[n], &len[n]);
        if (res != -1) {
            printf("\nQUST: A new player connected, ");
            fflush(stdout);
            sock[n] = res;
            n++;

            if (setsockopt(res, SOL_SOCKET, SO_REUSEADDR,
                &one, sizeof one) == -1) {
                perror("ERRO: player setsockopt");
            }
            // if (fcntl(res, F_SETFL, O_NONBLOCK) == -1) {
            //     perror("ERRO: fcntl");
            // }

            if (n < 2) {
                printf("at least one more is needed to start.\n");
                continue;
            } else {
                printf("start a game with %d players? [y/n]: ", n);
                fflush(stdout);
                flush_istream(fileno(stdin));
            }
        }
        if (errno != EAGAIN || errno != EWOULDBLOCK) {
            fprintf(stderr, "\n");
            perror("WARN: accept");
            continue;
        }
        sleep(2);
        if (n >= 2) {
            answ = fgetc(stdin);
            switch (answ) {
            case 'Y':
            case 'y':
                fd_unset_nonblocking(fileno(stdin), &stdin_fd_flags);
                return n;
            default:
                printf("ERRO: Invalid answer.\n");
            case 'N':
            case 'n':
                printf("QUST: Start a game with %d players? [y/n]: ", n);
                fflush(stdout);
                flush_istream(fileno(stdin));
            case -1:
                continue;
            }
        }
    }
    printf("\nINFO: Starting a game with 4 players.\n");
    fd_set_nonblocking(fileno(stdin), &stdin_fd_flags);
    return 4;
}
int serv_setup_game(struct Game_serv *g, int listen_sock) {

    int errors = 0;
    int n_players = 0;
    int ps_sock[4] = { 0 };
    struct sockaddr_in ps_addr[4] = { 0 };
    socklen_t ps_len[4] = { 0 };

    n_players = serv_wait_players(listen_sock, ps_sock, ps_addr, ps_len);

    // ask about having a team game or not
    bool teamgame_answer = false;
    if (n_players == 4) {
        char answ;
    prompt_teamgame:
        printf("QUST: Do you want to have a team game? [y/n]: ");
        flush_istream(fileno(stdin));
        answ = fgetc(stdin);
        switch (answ) {
        case 'Y':
        case 'y':
            // is_team_game = true;
            printf("ERRO: Not yet implemented\n");
            break;
        case 'N':
        case 'n':
            teamgame_answer = false;
            break;
        default:
            printf("ERRO: Invalid answer.\n");
            goto prompt_teamgame;
        }
    }
    // set other default values
    g->round = 0;
    g->pass = 0;
    memset(g->pass_cards, 0, sizeof(g->pass_cards));
    g->pass_suit = -1;
    g->pass_master = 0;
    g->turn_counter = 0;
    g->turn_idx = 0;
    g->player_count = n_players;
    g->team_game = teamgame_answer;
    g->target_score = DEFAULT_TARGET_SCORE;

    // setup virgin deck
    memset(&g->deck, 0, 40 * sizeof(struct Card));
    initialize_deck(g->deck);

    // setup player structs, start the relative recv threads and get their names
    memset(&g->players, 0, 4 * sizeof(struct Player));
    for (int i = 0; i < n_players; i++) {
        struct Player *p = &g->players[i];
        p->id = i;
        p->game_score = 0;
        p->hand.size = 0;
        p->netinfo.addr = ps_addr[i];
        p->netinfo.addr_len = ps_len[i];
        thread_recv_init(&p->netinfo.pk_queue, ps_sock[i]);
        errors += net_notify_clients(g, (int[]) { p->id }, 1, RQ_NAME, EV_WELCOME, &(struct EV_packet_welcome) { .id = p->id });
    }
    for (int i = 0; i < n_players; i++) {
        struct Player *p = &g->players[i];
        net_get_playername(p, PLAYERNAME_STRLEN);
    }
    return errors;
}
// TODO: forgor && kill recv threads
void server_end_game(struct Game_serv *g) {
    int winner_id = -1;
    int max_score = -1;

    int i_score = -1;
    for (int i = 0; i < g->player_count; i++) {
        i_score = g->players[i].game_score;
        if (i_score > max_score) {
            winner_id = i;
            max_score = i_score;
        }
    }

    assert(winner_id >= 0);
    assert(g->players[winner_id].game_score >= g->target_score);
    net_notify_clients(g, NULL, 0, RQ_NONE, EV_GAME_OVER, 
        &(struct EV_packet_gameover){.winner_id = winner_id});

    for (int i = 0; i < g->player_count; i++){
        llist_nuke(&g->players[i].hand, NULL);
        // TODO: clean everything else up

        // FIXME: test possible race conditions / other complications after sockets close
        close(g->players[i].netinfo.pk_queue.socket); 
        }
    printf("INFO: Game over.\nINFO: A new game will begin soon, please make everyone reconnect...\n");
}
int main(int argc, char **argv) {

    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        srand48(tv.tv_usec);
    }

    int res = 0,
        n_players = 0,
        listen_sock = 0;
    char *port_str = NULL;

    if (argc >= 2) {
        port_str = argv[1];
    }

    printf("INFO: Trying to bind to an address...\n");
    if ((listen_sock = serv_listen(port_str)) == -1) {
        exit(1);
    }

    while (true) {
        struct Game_serv game = { 0 };
        memset(&game, 0, sizeof(struct Game_serv));

        serv_setup_game(&game, listen_sock);

        net_notify_clients(&game, NULL, 0, RQ_NONE, EV_GAME_START, NULL);

        while (is_game_over_serv(game.players, game.player_count, game.target_score) == false) {
            give_cards(game.players, game.player_count, game.deck);

            // net_send_gameinfo(game.players, game.player_count, NULL, &game);
            // net_send_hand(game.players, game.player_count, NULL, game.round);

            serv_simulate_round(&game);
            game.round++;
        }
        server_end_game(&game);
    }
    return 0;
}
