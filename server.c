
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
    #include "windef.h"
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif // __MINGW32__

#include "common.h"
#include "network.h"
#include "server.h"

int admin_idx = 0;

void give_cards(struct Player_serv pl[4], int n_pl, struct Card *deck) {
    int shuffled_order[40];
    for (int i = 0; i < 40; i++) {
        shuffled_order[i] = i;
    }
    for (int i = 39; i > 0; i--) {
        if (/*n_pl == 3 &&*/ deck[shuffled_order[i]].suit == SPADE && deck[shuffled_order[i]].value == 5) {
            int k = shuffled_order[39];
            shuffled_order[i] = k;
            shuffled_order[39] = i;
        }
        int roll = (unsigned int) (drand48()*(i));
        int t = shuffled_order[roll];
        shuffled_order[roll] = shuffled_order[i];
        shuffled_order[i] = t;
    }

    int idx = 0;
    const int cards_pp = 40 / n_pl;
    for (int i = 0; i < n_pl; i++) {
        pl[i].round_score_thirds = 0;
        pl[i].card_count = cards_pp;
        llist_nuke(&pl[i].hand, NULL);
        struct Card_node *cnp;
        for (int j = 0; j < cards_pp; j++) {
            cnp = malloc(sizeof (struct Card_node));
            assert(cnp != NULL);
            cnp->c = &deck[shuffled_order[idx]];
            
            llist_add((llist*) &(pl[i].hand), (llist_node*) cnp);
            idx++;
        }
    }
}
void simulate_round(struct Player_serv players[4], const int n_players, const int n_round) {
    int cards_remaining = (40 / n_players) * n_players,
        starting_idx = n_round % n_players,
        i_pass = 0;

    struct Card **thrown_cards = calloc(n_players, sizeof(struct Card*));
    assert(thrown_cards != NULL);

    enum Suits main_suit = -1;
    int thrown_totval = -1, extra_point = -1,
        pass_winner_id = -1, turn_player_id = -1;

    struct Player_serv *turn_player = NULL;
    int turn_card_id = -1;
    struct Card_node *turn_card = NULL;
    
    while (cards_remaining > 0) {
        main_suit = -1;
        thrown_totval = pass_winner_id = turn_player_id = -1;
        memset(thrown_cards, 0, n_players * sizeof(struct Card*));

        extra_point = 3 * (cards_remaining == n_players); // last pass is worth 1 more full point

        printf("TRAC: Round %d, Pass %d\n", n_round+1, i_pass+1);
        for (int i_turn = 0; i_turn < n_players; i_turn++) {
            turn_card = NULL;
            turn_player_id = (starting_idx + i_turn) % n_players;
            turn_player = &players[turn_player_id];
            turn_card_id = net_get_playermove(turn_player, n_round, i_pass);
            assert(turn_card_id != -1);
            do {

                turn_card = find_valid_card((struct Card_node*)turn_player->hand.head, turn_player->card_count, turn_card_id, main_suit);
            } while (turn_card == NULL);
            printf("NOTE: Played card is %d of %s:%d\n", turn_card->c->value+1, suit_to_string(turn_card->c->suit), turn_card->c->value);
            if (i_turn == 0) { main_suit = turn_card->c->suit; };
            int moveresult_args[4] = { turn_player_id, turn_card_id, n_round, i_pass};
            net_send_moveresult(players, n_players, NULL, (struct Packet_moveresult) {
                .round = n_round,
                .pass = i_pass,
                .card = { .id = 0, .suit = turn_card->c->suit, .val = turn_card->c->value }, // id is not relevant
                .player_id = turn_player_id});
            thrown_cards[turn_player_id] = turn_card->c;
            llist_remove(&turn_player->hand, turn_card);
            turn_player->card_count -= 1;
        }
        // all turns over
        thrown_totval = calculate_pass_value(thrown_cards, n_players) + extra_point;
        pass_winner_id = select_pass_winner(thrown_cards, n_players, starting_idx);
        players[pass_winner_id].round_score_thirds += thrown_totval;

        printf("TRAC: %s:%d won %d", players[pass_winner_id].name, pass_winner_id+1, thrown_totval / 3);
        if (thrown_totval % 3) printf(".%d", (thrown_totval % 3) * 33);
        printf(" points with %d of %s --\n", thrown_cards[pass_winner_id]->value + 1, 
            suit_to_string(thrown_cards[pass_winner_id]->suit));

        net_send_outcome(players, n_players, NULL, (struct Packet_outcome) {
            .round = n_round,
            .pass = i_pass,
            .winner_id = pass_winner_id,
            .n_scores = n_players,
            .scores = { players[0].round_score_thirds, 
                       players[1].round_score_thirds, 
                       players[2].round_score_thirds, 
                       players[3].round_score_thirds }});

        starting_idx = pass_winner_id;
        cards_remaining -= n_players;
        i_pass++;
    }
    // all passes over

    for(int i = 0; i < n_players; i++) {
        players[i].game_score = (int) (players[i].round_score_thirds / 3);
    }
    // round over
    net_send_leaderboard(players, n_players, NULL, (struct Packet_leaderboard) {
        .round = n_round,
        .n_players = n_players,
        .scores = { players[0].game_score, 
                    players[1].game_score, 
                    players[2].game_score, 
                    players[3].game_score}});

    free(thrown_cards);
}
int select_pass_winner(struct Card **thrown, int n_thrown, int first_player_id) {
    assert(first_player_id < n_thrown);
    enum Suits main_suit = thrown[first_player_id]->suit;
    int winning_player = first_player_id;
    int winning_strenght = thrown[first_player_id]->value;
    printf("TRAC: Main Suit is %s, from Player %d.", suit_to_string(main_suit), first_player_id);
    int card_strenght;
    for (int i = 0; i < n_thrown; i++) {
        printf("TRAC: %d played %d of %s", i, thrown[i]->value + 1, suit_to_string(thrown[i]->suit));

        card_strenght = thrown[i]->value;
        // 0, 1 and 2 (ace, two and three) will have a strenght of 10, 11 and 12 respectively
        if (card_strenght <= 2) { card_strenght += 10; printf(" upgraded to %d", card_strenght); }

        if (thrown[i]->suit == main_suit && card_strenght > winning_strenght) {
            winning_strenght = card_strenght;
            winning_player = i;
        }
        printf("\n");
    }
    printf("\n");
    return winning_player;
}
bool is_game_over_serv(struct Player_serv *ps, int n_players, int target) {
    for (int i = 0; i < n_players; i++)
        if (ps[i].game_score >= target)
            return true;
    return false;
}

void net_get_playername(struct Player_serv *p, int bufsiz) {
    struct PNode *pn = net_need_packetkind(NAME, &p->netinfo.pk_q, 0, true, p->netinfo.sock_fd);
    if (pn == NULL) {
        goto noname_player;
    }
    struct Packet_name *pnm = (struct Packet_name*) pn->pk->data;
    int maxlen = (pnm->size < bufsiz) ? pnm->size : bufsiz-1;
    if (pnm->name[0] == '\0') {
        free(pn->pk);
        llist_remove(&p->netinfo.pk_q, pn);
        goto noname_player;
    }
    for (int i = 0; i < maxlen; i++) {
        if (pnm->name[i] == '\0' || pnm->name[i] == '\n') { p->name[i] = '\0'; break; }
        if (pnm->name[i] >= 32 || pnm->name[i] <= 126)      p->name[i] = pnm->name[i];
        else                                                p->name[i] = '.';
    }
    printf("TRAC: Player %d's name is %s\n", p->id, p->name);
    free(pn->pk);
    llist_remove(&p->netinfo.pk_q, pn);
    return;

    noname_player:
    snprintf(p->name, bufsiz-1, "Player %d", p->id + 1);
    printf("INFO: Player %d didn't give a name\n", p->id);
    return;
}
int net_get_playermove(struct Player_serv *p, int round, int pass) {
    int card_id = -1;
    int deltaround, deltapass;
    struct PNode *pn;
    struct Packet_move *pmv;
    int i = 0;

    for (int i = 0; i < LOOKFOR_MAXPACKETS; i++) {
        pn = net_need_packetkind(MOVE, &p->netinfo.pk_q, i, true, p->netinfo.sock_fd);
        assert(pn != NULL);
        pmv = (struct Packet_move*) pn->pk->data;
        deltaround = pmv->round - round;
        deltapass = pmv->pass - pass;
        if (deltaround < 0 || (deltaround == 0 && deltapass < 0)) {
            printf("INFO: Found an old MOVE packet (from %d:%s on %d:%d)", p->id, p->name, pmv->round+1, pmv->pass+1);
            free(pn->pk);
            llist_remove(&p->netinfo.pk_q, pn);
            i--;
            continue;
        }
        if ((deltaround == 0) && (deltapass == 0)) {
            card_id = pmv->card_id;
            printf("INFO: Got a MOVE packet from %d:%s (card_id %d)\n", p->id, p->name, card_id);
            free(pn->pk);
            llist_remove(&p->netinfo.pk_q, pn);
            return card_id;
        }
    }
    return -1;
}

int net_send_playerinfo(struct Player_serv *p) {
    struct Packet *packet = calloc(1, PACKET_SIZE);
    assert(packet != NULL);
    packet->kind = PLAYERINFO;
    packet->size = sizeof (struct Packet_playerinfo);
    struct Packet_playerinfo *ppi = (struct Packet_playerinfo*)packet->data;
    ppi->id = p->id;
    struct Packet_name *pn = &ppi->name;
    strncpy(pn->name, p->name, PLAYERNAME_STRLEN-1);
    pn->size = strlen(p->name) + 1;

    char *ptr;
    int sock = p->netinfo.sock_fd;
    int ret = net_send_packet(sock, packet);
    printf("INFO: Sending a PLAYERINFO packet to %d:%s\n", p->id, p->name);
    free(packet);
    return ret;
}
int net_send_gameinfo(struct Player_serv *ps, int np, int *whom, struct Game_serv *game) {
    int whom_all[] = {0, 1, 2, 3};
    struct Packet *packet;
    struct Packet_gameinfo *pgi;

    packet = calloc(1, PACKET_SIZE);
    packet->kind = GAMEINFO;
    packet->size = sizeof (struct Packet_gameinfo);
    pgi = (struct Packet_gameinfo*)packet->data;
    pgi->player_count = game->player_count;
    pgi->round = game->round;
    pgi->team_game = game->team_game;
    pgi->target_score = game->target_score;
    for (int i = 0; i < game->player_count; i++) {
        pgi->players[i].id = i;
        pgi->players[i].name.size = strlen(ps[i].name) + 1;
        strncpy(pgi->players[i].name.name, ps[i].name, strlen(ps[i].name));
    }

    if (whom == NULL) {
        whom = whom_all;
    }
    int errors = 0, sock = 0;
    for (int i = 0; i < np; i++) {
        sock = ps[whom[i]].netinfo.sock_fd;
        printf("INFO: Sending a GAMEINFO packet to %d:%s\n", ps[whom[i]].id, ps[whom[i]].name);
        errors += net_send_packet(sock, packet);
    }
    free(packet);
    return errors;
}
int net_send_hand(struct Player_serv *ps, int np, int *whom, int n_round) {
    int sock;
    int errors = 0;
    int whom_all[] = {0, 1, 2, 3};
    struct Packet *packet;
    struct Packet_hand *phn;
    struct Packet_card *pc_ptr;

    // if (np < 4 && whom == NULL) {
    //     fprintf(stderr, "ERRO: have to specify who to send cards to if np<4.\n");
    //     return;
    // }
    if (whom == NULL) {
        whom = whom_all;
    }
    struct Card_node *cn;
    for (int i = 0; i < np; i++) {
        packet = calloc(1, PACKET_SIZE);
        packet->kind = HAND;
        packet->size = sizeof (struct Packet_hand);
        phn = (struct Packet_hand*)packet->data;
        phn->round = n_round;
        phn->n_cards = ps[i].card_count;
        assert(phn->n_cards <= 20);

        cn = (struct Card_node*) ps[i].hand.head;
        for (int ci = 0; ci < phn->n_cards; ci++) {
            assert(cn != NULL);
            pc_ptr = &phn->cards[ci];
            printf("DEBG: %d Giving %d of %s%d to player %d.\n", ci, cn->c->value+1, suit_to_string(cn->c->suit), cn->c->suit, i);
            pc_ptr->id = ci;
            pc_ptr->suit = cn->c->suit;
            pc_ptr->val = cn->c->value;

            cn = (struct Card_node*)cn->node.next;
        }

        sock = ps[whom[i]].netinfo.sock_fd;
        printf("INFO: Sending a HAND packet to %d:%s\n", ps[whom[i]].id, ps[whom[i]].name);
        errors += net_send_packet(sock, packet);
        free(packet);
    }
    return errors;
}
int net_send_moveresult(struct Player_serv *ps, int np, int *whom, struct Packet_moveresult args) {
    int whom_all[] = {0, 1, 2, 3};
    
    struct Packet *packet = calloc(1, PACKET_SIZE);
    packet->kind = MOVERESULT;
    packet->size = sizeof (struct Packet_moveresult);
    struct Packet_moveresult *pmr = (struct Packet_moveresult*)packet->data;
    pmr->round = args.round;
    pmr->pass = args.pass;
    pmr->player_id = args.player_id;
    pmr->card.id = args.card.id;
    pmr->card.suit = args.card.suit;
    pmr->card.val = args.card.val;

    if (whom == NULL) {
        whom = whom_all;
    }
    int errors = 0, sock = 0;
    for (int i = 0; i < np; i++) {
        sock = ps[whom[i]].netinfo.sock_fd;
        printf("INFO: Sending a MOVERESULT packet to %d:%s\n", ps[whom[i]].id, ps[whom[i]].name);
        errors += net_send_packet(sock, packet);
    }
    free(packet);
    return errors;
}
int net_send_outcome(struct Player_serv *ps, int np, int *whom, struct Packet_outcome args) {
    int whom_all[] = {0, 1, 2, 3};

    struct Packet *packet = calloc(1, PACKET_SIZE);
    assert(packet != NULL);
    packet->kind = OUTCOME;
    packet->size = sizeof (struct Packet_outcome);
    struct Packet_outcome *poc = (struct Packet_outcome*)packet->data;
    poc->round = args.round;
    poc->pass = args.pass;
    poc->winner_id = args.winner_id;
    poc->n_scores = args.n_scores;
    for (int i = 0; i < poc->n_scores; i++)
        poc->scores[i] = args.scores[i];

    if (whom == NULL) {
        whom = whom_all;
    }
    int errors = 0, sock = 0;
    for (int i = 0; i < np; i++) {
        sock = ps[whom[i]].netinfo.sock_fd;
        printf("INFO: Sending a OUTCOME packet to %d:%s\n", ps[whom[i]].id, ps[whom[i]].name);
        errors += net_send_packet(sock, packet);
    }
    free(packet);
    return errors;
}
int net_send_leaderboard(struct Player_serv *ps, int np, int *whom, struct Packet_leaderboard args) {
    int whom_all[] = {0, 1, 2, 3};

    struct Packet *packet = calloc(1, PACKET_SIZE);
    assert(packet != NULL);
    packet->kind = LEADERBOARD;
    packet->size = sizeof (struct Packet_leaderboard);
    struct Packet_leaderboard *plb = (struct Packet_leaderboard*)packet->data;
    plb->round = args.round;
    plb->n_players = args.n_players;
    for (int i = 0; i < args.n_players; i++)
        plb->scores[i] = ps[i].game_score;

    if (whom == NULL) {
        whom = whom_all;
    }
    int errors = 0, sock = 0;
    for (int i = 0; i < np; i++) {
        sock = ps[whom[i]].netinfo.sock_fd;
        printf("INFO: Sending a LEADERBOARD packet to %d:%s\n", ps[whom[i]].id, ps[whom[i]].name);
        errors += net_send_packet(sock, packet);
    }
    free(packet);
    return errors;
}
int net_send_coronation(struct Player_serv *ps, int np, int *whom, int winner_id) {
    int whom_all[] = {0, 1, 2, 3};

    struct Packet *packet = calloc(1, PACKET_SIZE);
    assert(packet != NULL);
    packet->kind = CORONATION;
    packet->size = sizeof (struct Packet_coronation);
    struct Packet_coronation *pcn = (struct Packet_coronation*)packet->data;
    pcn->winner_id = winner_id;

    if (whom == NULL) {
        whom = whom_all;
    }
    int errors = 0, sock = 0;
    for (int i = 0; i < np; i++) {
        sock = ps[whom[i]].netinfo.sock_fd;
        printf("INFO: Sending a CORONATION packet to %d:%s\n", ps[whom[i]].id, ps[whom[i]].name);
        errors += net_send_packet(sock, packet);
    }
    free(packet);
    return errors;
}


int serv_listen(int port) {
    int sockfd = -1, res, one = 1;
    struct addrinfo hints, *ai_res, *ain;
    char port_str[8];
    snprintf(port_str, 5, "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if ((res = getaddrinfo(NULL, port_str, &hints, &ai_res)) != 0) {
        fprintf(stderr, "FATL: gai = %s\n", gai_strerror(res));
        return -1;
    }
    
    for (ain = ai_res; ain != NULL; ain = ain->ai_next) {
        if ((res = socket(ain->ai_family, ain->ai_socktype, ain->ai_protocol)) == -1) {
            perror("WARN: socket");
            continue;
        };
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
int net_wait_players(int listen_sock, int *sock, struct sockaddr_in *addr, socklen_t *len) {
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
            }
            else {
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
                case 'Y': case 'y':
                    fd_unset_nonblocking(fileno(stdin), &stdin_fd_flags);
                    return n;
                default:
                    printf("ERRO: Invalid answer.\n");
                case 'N': case 'n':
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
int serv_setup_game(struct Game_serv *g, int listen_sock, int *sock, struct sockaddr_in *addr, socklen_t *len) {
    int errors = 0;
    int n_players = net_wait_players(listen_sock, sock, addr, len);
    bool is_team_game = false;

    if (n_players == 4) {
        char answ;
        prompt_teamgame:
        printf("QUST: Do you want to have a team game? [y/n]: ");
        flush_istream(fileno(stdin));
        answ = fgetc(stdin);
        switch (answ) {
            case 'Y': case 'y':
                // is_team_game = true;
                printf("ERRO: Not yet implemented\n");
                break;
            case 'N': case 'n':
                is_team_game = false;
                break;
            default:
                printf("ERRO: Invalid answer.\n");
                goto prompt_teamgame;
        }
    }


    g->round = 0;
    g->player_count = n_players;
    g->team_game = is_team_game;
    g->target_score = DEFAULT_TARGET_SCORE;

    memset(&g->deck, 0, 40 * sizeof(struct Card));
    initialize_deck(g->deck);
        
    memset(&g->players, 0, 4 * sizeof(struct Player_serv));
    for (int i = 0; i < n_players; i++) {
        g->players[i].id = i;
        g->players[i].game_score = 0;
        g->players[i].hand.size = 0;
        g->players[i].netinfo.sock_fd = sock[i];
        g->players[i].netinfo.addr = addr[i];
        g->players[i].netinfo.addr_len = len[i];
        g->players[i].netinfo.pk_q.size = 0;
        g->players[i].netinfo.pk_q.head = g->players[i].netinfo.pk_q.tail = 
                (struct llist_node *) &g->players[i].netinfo.pk_q;
        net_get_playername(&g->players[i], PLAYERNAME_STRLEN);
        printf("INFO: Player %d will call themselves %s.\n", i, g->players[i].name);
        errors += net_send_playerinfo(&g->players[i]);
    }
    return errors;
}
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
    net_send_coronation(g->players, g->player_count, NULL, winner_id);

    for (int i = 0; i < g->player_count; i++)
        close(g->players[i].netinfo.sock_fd);
    printf("INFO: Game over.\nINFO: A new game will begin soon, please make everyone reconnect...\n");
}
int main(int argc, char **argv) {

    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand48(tv.tv_usec);

    int res = 0,
        n_players = 0, 
        port, 
        listen_sock = 0;

    if (argc >= 2) {
        port = strtol(argv[1], NULL, 10);
    }
    if (port == 0) port = PORT_DEFAULT;

    printf("INFO: Trying to bind to an address...\n");
    if ((listen_sock = serv_listen(port)) == -1) {
        exit(1);
    }

    while (true) {
        struct Game_serv game;
        memset(&game, 0, sizeof(struct Game_serv));
        int player_sock[4] = {0};
        struct sockaddr_in player_sock_addr[4] = {0};
        socklen_t player_sock_len[4] = {0};

        serv_setup_game(&game, listen_sock, player_sock, player_sock_addr, player_sock_len);

        while (is_game_over_serv(game.players, game.player_count, game.target_score) == false) {
            give_cards(game.players, game.player_count, game.deck);

            net_send_gameinfo(game.players, game.player_count, NULL, &game);
            net_send_hand(game.players, game.player_count, NULL, game.round);

            simulate_round(game.players, game.player_count, game.round);
            game.round++;
        }
        server_end_game(&game);
    }
    return 0;
}
