#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
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
#include "client.h"
#include "network.h"
int admin_idx = 0;

void dbg_selection(struct Card_node *head, int n_cards, int *selector_arr, int selection_id) {
    struct Card_node *cn = NULL;
    for (int i = 0; i < n_cards; i++) {
        printf("Selection %d -> %d: ", i+1, selector_arr[i]);
        cn = head;
        for (int j = 0; j < n_cards; j++) {
            if (selector_arr[i] == j) {
                printf("%d of %s:%d\n", cn->c->value+1, suit_to_string(cn->c->suit), cn->c->suit);
                if (i == selection_id) printf("^^^\t this is what you played.\n");
                break;
            }
            cn = (struct Card_node*)cn->node.next;
        }
    }
}
                

// void simulate_round(struct Player *players, int n_players, int n_round) {
//     struct Card **thrown = calloc(n_players, sizeof(struct Card*));
//     assert(thrown != NULL);
//     struct Card_node *cn;
//     int cards_remaining = (40 / n_players) * n_players;
//     int starting_idx = n_round % n_players;
//     int i = 0;
//     while (cards_remaining > 0) {
//         enum Suits main_suit = -1;
//         int winning_idx, pass_score, curr_player_idx;
//         int extra_point = 3 * (cards_remaining == n_players); // last pass is worth 1 more full point
//         memset(thrown, 0, n_players * sizeof(struct Card*));
//         printf("---------------\n--- Round %d ---\n---------------\n", ++i);
//         for (int i = 0; i < n_players; i++) {
//             curr_player_idx = (starting_idx + i) % n_players;
//             while (true) {
//                 cn = take_player_card(&players[curr_player_idx], main_suit);
//                 if (cn != NULL)  {
//                     if (i == 0) { main_suit = cn->c->suit; };
//                     break;
//                 }
//             };
//             thrown[curr_player_idx] = cn->c;
//             llist_remove(cn);
//         }
//         winning_idx = select_pass_winner(thrown, n_players, starting_idx);
//         pass_score = calculate_pass_score(thrown, n_players) + extra_point; 
//         players[winning_idx].round_score_thirds += pass_score;
//         printf("--- Player %d won with %d of %s, got %d", winning_idx+1, thrown[winning_idx]->value, suit_to_string(thrown[winning_idx]->suit), pass_score / 3);
//         if (pass_score % 3) printf(".%d", (pass_score % 3) * 33);
//         printf(" points ---\n");
//         starting_idx = winning_idx;
//         cards_remaining -= n_players;
//     }
//     // round over
//     for(int i = 0; i < n_players; i++) {
//         players[i].game_score = (int) (players[i].round_score_thirds / 3);
//     }
//     free(thrown);
// }

int sort_card_packets(const void *c1, const void *c2) {
    return ((struct Packet_card*) c1)->val - ((struct Packet_card*) c2)->val;
}

int client_prompt_name(char *name, int maxlen) {
    size_t bufsiz = maxlen+2;
    char *buf = calloc(bufsiz, sizeof(char));
    int ret; 

    
    printf("What would you like to call yourself? (max %d characters) ", maxlen);
    flush_istream(fileno(stdin));
    getline(&buf, &bufsiz, stdin);
    // fgets(buf, maxlen+1, stdin);
    for (int i = 0; i < maxlen; i++) {
        if (buf[i] == '\0' || buf[i] == '\n') {
            name[i] = '\0';
            break;
        }
        name[i] = buf[i];
    }
    name[maxlen] = '\0';
    ret = strlen(name);
    free(buf);
    return ret;
}
int client_prompt_move(struct Player_client *p, enum Suits main_suit) {
    printf("Which card do you want to play? ");
    int selection = -1;
    size_t bufsiz = 8;
    char *buf_str = calloc(bufsiz, sizeof(char));
    char *start_ptr, *end_ptr;
    while (true) {
        flush_istream(fileno(stdin));
        getline(&buf_str, &bufsiz, stdin);
        buf_str[2] = '\0';
        start_ptr = buf_str;
        do {
            selection = strtol(start_ptr, &end_ptr, 10);
        } while (end_ptr == start_ptr++);

        if (selection > 0 && selection <= p->card_count) {
            break;
        }

        printf("\nInvalid input, retry: ");
        fflush(stdout);
        continue;
    }
    free(buf_str);

    // assert(selection >= 0 && selection < p->card_count);
    return selection - 1;
}

int net_send_playername(struct Game_client *game, char *name, int name_len) { // ret: 0 success, -1 error
    struct Packet *packet = calloc(1, PACKET_SIZE);
    assert(packet != NULL);

    packet->kind = NAME;
    packet->size = sizeof (struct Packet_name);
    struct Packet_name *pnm = (struct Packet_name*)packet->data;
    pnm->size = name_len + 1;
    strncpy(pnm->name, name, name_len);

    int ret = net_send_packet(game->socket, packet);
    free(packet);
    return ret;
}
int net_send_playermove(struct Game_client *game, int card_id) { // ret: 0 success, -1 error
    struct Packet *packet = calloc(1, PACKET_SIZE);
    assert(packet != NULL);

    packet->kind = MOVE;
    packet->size = sizeof (struct Packet_move);
    struct Packet_move *pmv = (struct Packet_move*)packet->data;
    pmv->round = game->round;
    pmv->pass = game->pass;
    pmv->card_id = card_id;

    int ret = net_send_packet(game->socket, packet);
    free(packet);
    return ret;
}

void net_get_playerinfo(struct Game_client *game) {
    struct PNode *pn = net_need_packetkind(PLAYERINFO, &game->queue, 0, true, game->socket);
    assert(pn != NULL);

    struct Packet_playerinfo *ppi = (struct Packet_playerinfo*) pn->pk->data;
    int maxlen = ( ppi->name.size - 1 < PLAYERNAME_STRLEN ) ? ppi->name.size : PLAYERNAME_STRLEN;
    game->my_id = ppi->id;
    strncpy(game->players[game->my_id].name, ppi->name.name, maxlen);
    
    free(pn->pk);
    llist_remove(&game->queue, pn);
    return;
}
void net_get_gameinfo(struct Game_client *game) {
    struct PNode *pn = net_need_packetkind(GAMEINFO, &game->queue, 0, true, game->socket);
    assert(pn != NULL);

    struct Packet_gameinfo *pgi = (struct Packet_gameinfo*) pn->pk->data;
    game->player_count = pgi->player_count;
    game->team_game = pgi->team_game;
    game->target_score = pgi->target_score;
    game->round = pgi->round;
    game->pass = 0; // FIXME: is this a good place to set this?
    int maxlen;
    struct Player_client *cp;
    for (int i = 0; i < pgi->player_count; i++) {
        cp = &game->players[i];
        cp->id = pgi->players[i].id;
        maxlen = (pgi->players[i].name.size - 1 < PLAYERNAME_STRLEN)
                    ? pgi->players[i].name.size - 1
                    : PLAYERNAME_STRLEN;
        strncpy(cp->name, pgi->players[i].name.name, maxlen);
        cp->card_count = 40 / pgi->player_count;
        cp->game_score = 0;
        cp->round_score_thirds = 0;
        cp->hand.size = 0;
        cp->hand.head = cp->hand.head = (llist_node*)&cp->hand;
    }

    free(pn->pk);
    llist_remove(&game->queue, pn);
    return;
}
int net_get_hand(struct Game_client *game) {
    struct PNode *pn;
    struct Packet_hand *phd;
    struct Player_client *p = &game->players[game->my_id];
    int i = 0;
    int deltaround;

    for (int i = 0; i < LOOKFOR_MAXPACKETS; i++) {
        pn = net_need_packetkind(HAND, &game->queue, 0, true, game->socket);
        assert(pn != NULL);
        phd = (struct Packet_hand*) pn->pk->data;
        deltaround = phd->round - game->round;
        if (deltaround < 0) {
            free(pn->pk);
            llist_remove(&game->queue, pn);
            i--;
            continue;
        }
        if (deltaround == 0) {
            p->card_count = phd->n_cards;
            llist_nuke(&p->hand, NULL);

            struct Card_node *cn;
            struct Packet_card incoming_card;
            enum Suits ic_suit;
            int ic_val;
            for (int i = 0; i < p->card_count; i++) {
                cn = calloc(1, sizeof(struct Card_node));
                assert(cn != NULL);
                assert(phd->cards[i].id == i); // TEMP
                ic_suit = phd->cards[i].suit;
                ic_val = phd->cards[i].val;
                cn->c = &game->deck[ic_suit * 10 + ic_val];
                llist_append(&p->hand, (llist_node*)cn);
            }
            free(pn->pk);
            llist_remove(&game->queue, pn);
            return 0;
        }
    }
    return -1;
}
struct Card *net_get_moveresult(struct Game_client *game) { // deck card pointer success, NULL could not find packet
    struct PNode *pn;
    struct Packet_moveresult *pmr;
    int i = 0;
    int deltaround, deltapass;
    int retsuit, retval;

    for (int i = 0; i < LOOKFOR_MAXPACKETS; i++) {
        pn = net_need_packetkind(MOVERESULT, &game->queue, i, true, game->socket);
        assert(pn != NULL);
        pmr = (struct Packet_moveresult*) pn->pk->data;
        deltaround = pmr->round - game->round;
        deltapass = pmr->pass - game->pass;
        if (deltaround < 0 || (deltaround == 0 && deltapass < 0)) {
            free(pn->pk);
            llist_remove(&game->queue, pn);
            i--;
            continue;
        }
        if ((deltaround == 0 && deltapass == 0) /*&& (pmr->player_id == game->turn_idx)*/) {
            retsuit = pmr->card.suit;
            retval = pmr->card.val;
            // game->players[pmr->player_id].card_count--;
            free(pn->pk);
            llist_remove(&game->queue, pn);
            return &game->deck[retsuit * 10 + retval];
        }
    }
    return NULL;
}
int net_get_outcome(struct Game_client *game) { // ret: winner_id success; -1 could not find packet
    struct PNode *pn;
    struct Packet_outcome *poc;
    int i = 0;
    int deltaround, deltapass;
    int winner_id = -1;

    for (int i = 0; i < LOOKFOR_MAXPACKETS; i++) {
        pn = net_need_packetkind(OUTCOME, &game->queue, i, true, game->socket);
        assert(pn != NULL);
        poc = (struct Packet_outcome*) pn->pk->data;
        deltaround = poc->round - game->round;
        deltapass = poc->pass - game->pass;
        if (deltaround < 0 || (deltaround == 0 && deltapass < 0)) {
            free(pn->pk);
            llist_remove(&game->queue, pn);
            i--;
            continue;
        }
        if ((deltaround == 0) && (deltapass == 0)) {
            for (int i = 0; i < poc->n_scores; i++) {
                game->players[i].round_score_thirds = poc->scores[i];
            }
            winner_id = poc->winner_id;
            free(pn->pk);
            llist_remove(&game->queue, pn);
            return winner_id;
        }
    }
    return -1;
}
int net_get_leaderboard(struct Game_client *game) { // ret: 0 success; -1 couldnt find packet
    struct PNode *pn;
    struct Packet_leaderboard *plb;
    int i = 0;
    int deltaround;
    int winner_id = -1;

    for (int i = 0; i < LOOKFOR_MAXPACKETS; i++) {
        pn = net_need_packetkind(LEADERBOARD, &game->queue, i, true, game->socket);
        assert(pn != NULL);
        plb = (struct Packet_leaderboard*) pn->pk->data;
        deltaround = plb->round - game->round;
        if (deltaround < 0 ) {
            free(pn->pk);
            llist_remove(&game->queue, pn);
            i--;
            continue;
        }
        if (deltaround == 0) {
            for (int i = 0; i < plb->n_players; i++) {
                game->players[i].game_score = plb->scores[i];
            }
            free(pn->pk);
            llist_remove(&game->queue, pn);
            return 0;
        }
    }
    return -1;
}
int net_get_coronation(struct Game_client *game) { // ret: winner_id
    struct PNode *pn;
    struct Packet_coronation *pcn;
    int winner_id = -1;

    pn = net_need_packetkind(CORONATION, &game->queue, 0, true, game->socket);
    assert(pn != NULL);
    pcn = (struct Packet_coronation*) pn->pk->data;

    winner_id = pcn->winner_id;

    free(pn->pk);
    llist_remove(&game->queue, pn);
    return winner_id;
}

int client_connect(char *addr, char *port) {
    int sockfd = -1, res = 0, one = 1;
    struct addrinfo hints, *ai_res, *ain;
    char default_port[8];
    assert(addr != NULL);
    if (port == NULL) {
        snprintf(default_port, 5, "%d", PORT_DEFAULT);
        port = default_port;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if ((res = getaddrinfo(addr, port, &hints, &ai_res)) != 0) {
        fprintf(stderr, "FATL: gai = %s\n", gai_strerror(res));
        return -1;
    }
    
    for (ain = ai_res; ain != NULL; ain = ain->ai_next) {
        if ((res = socket(ain->ai_family, ain->ai_socktype, ain->ai_protocol)) == -1) {
            perror("WARN: socket");
            continue;
        };

        if (setsockopt(res, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) == -1) {
            perror("ERRO: setsockopt");
        }
        
        if (connect(res, ain->ai_addr, ain->ai_addrlen) == 0) {
            // if (fcntl(res, F_SETFL, O_NONBLOCK) == -1) {
            //     perror("ERRO: fcntl");
            // }
            sockfd = res;
            break;
        }

        if (shutdown(res, SHUT_RDWR) == -1) {
            perror("WARN: shutdown");
        }
        close(res);
        res = -1;
        break;
    }

    freeaddrinfo(ai_res);

    if (ain == NULL || res == -1) {
        fprintf(stderr, "FATL: Could not connect to server...\n");
        return -1;
    }
    return sockfd;
}

void client_setup_game(struct Game_client *game) {
    game->queue.head = (llist_node*)&game->queue;
    game->queue.tail = (llist_node*)&game->queue;
    game->queue.size = 0;

    char name[PLAYERNAME_STRLEN+1];
    int name_len = client_prompt_name(name, PLAYERNAME_STRLEN);
    net_send_playername(game, name, name_len);

    printf("Waiting for acknowledgement...");
    fflush(stdout);
    net_get_playerinfo(game);
    printf(" acknowledged, your id is %d.\nWaiting for the game to start...", game->my_id);
    fflush(stdout);
    net_get_gameinfo(game);
    printf(" game is starting!\n");

    initialize_deck(game->deck);
}

void game_print_roundpass(int round, int pass) {
    char extra_dashes[3] = "\0\0\0";
    int dash_i = 0;
    if (round / 10) extra_dashes[dash_i++] = '-';
    if (pass / 10) extra_dashes[dash_i++] = '-';
    printf("\t\t\t\t\t\b\b\b------------------------%s\n", extra_dashes);
    printf("\t\t\t\t\t\b\b\b--- Round %d - Pass %d ---\n", round+1, pass+1);
    printf("\t\t\t\t\t\b\b\b------------------------%s\n", extra_dashes);
}
void game_print_hand(struct Card_node *head, int n_cards, int selector_arr[20]) {
    struct Packet_card buf[4][20];
    memset(buf, 0, 80 * sizeof(struct Packet_card));
    int sizes[4] = {0};
    int suits_remaining_tot = 4;
    char suits_remaining[5] = "AAAA";

    enum Suits cs = -1;
    struct Card_node *cn = head;
    for (int i = 0; i < n_cards; i++) {
        assert(cn != NULL);
        cs = cn->c->suit;
        buf[cs][sizes[cs]].id = i;
        buf[cs][sizes[cs]].suit = cs;
        buf[cs][sizes[cs]].val = cn->c->value;
        sizes[cs]++;
        cn = (struct Card_node*)cn->node.next;
    }
    for (int i = 0; i < 4; i++) {
        qsort(buf[i], sizes[i], sizeof(struct Packet_card), sort_card_packets);
    }
    
    int i_card = 0;
    int line = 0;
    char selector_str[16] = "";
    bool with_selection = (selector_arr != NULL);
    printf(".\t.\t.\t.\t.\t.   Your Hand    .\t.\t.\t.\t.\t.\n\n");
    printf("\tDenara\t\t\tCoppe\t\t\tBastoni\t\t\tSpade\n");
    while (memcmp(suits_remaining, "\0\0\0\0", 4) != 0) {
        for (int i_suit = 0; i_suit < 4; i_suit++) {
            if (line < sizes[i_suit]) {
                if (with_selection == true) {
                    snprintf(selector_str, 15, "\b\b\b(%s%d) ", (i_card<9)?" ":"", i_card+1);
                    selector_arr[i_card] = buf[i_suit][line].id;
                    i_card++;
                }
                printf("\t%s%d of %s\t", selector_str,
                    buf[i_suit][line].val + 1, suit_to_string(buf[i_suit][line].suit));
            } else {
                printf("\t\t%s", with_selection ? "\t" : "");
                suits_remaining[i_suit] = '\0';
            }
        }
        printf("\n");
        line++;
    }
}

int client_play_round(struct Game_client *g) {

    net_get_hand(g);

    struct Card **thrown_cards = calloc(g->player_count, sizeof(struct Card*));
    assert(thrown_cards != NULL);
    int cards_remaining = (40 / g->player_count) * g->player_count;

    struct Player_client *p = &g->players[g->my_id];
    int player_card_id;
    struct Card_node *player_card = NULL;
    int *selector_arr = calloc(p->card_count, sizeof(int));

    enum Suits pass_suit;
    int turn_idx = g->round % g->player_count;
    struct Card *turn_card = NULL;

    int pass_winner_id;
    int thrown_totval;
    int extra_point;
    
    while (cards_remaining > 0) {
        memset(selector_arr, 0, p->card_count * sizeof(int));
        memset(thrown_cards, 0, g->player_count * sizeof(struct Card*));
        pass_suit = -1;
        extra_point = 3 * (cards_remaining == g->player_count); // last pass is worth 1 full point more

        game_print_roundpass(g->round, g->pass);
        game_print_hand((struct Card_node*)p->hand.head, p->card_count, selector_arr);
        for (int i = 0; i < g->player_count; i++) {
            player_card = NULL;
            turn_card = NULL;
            player_card_id = -1;

            if (turn_idx == p->id) {
                int selection_id = -1;
                while (player_card == NULL) {
                    selection_id = client_prompt_move(p, pass_suit);
                    player_card_id = selector_arr[selection_id];
                    player_card = find_valid_card((struct Card_node*)p->hand.head, p->card_count, player_card_id, pass_suit);
                }
                // dbg_selection((struct Card_node*)p->hand.head, p->card_count, selector_arr, selection_id);
                assert(net_send_playermove(g, player_card_id) == 0);
                llist_remove(&p->hand, player_card);
                turn_card = net_get_moveresult(g);
                printf("You played %d of %s.\n", turn_card->value+1, suit_to_string(turn_card->suit));
            } else {
                printf("It's %s's turn.\n", g->players[turn_idx].name);
                turn_card = net_get_moveresult(g);
                printf("%s played %d of %s.\n", g->players[turn_idx].name, 
                    turn_card->value+1, suit_to_string(turn_card->suit));
            }
            if (i == 0)  {
                pass_suit = turn_card->suit;
                printf("This pass will be played on %s.\n", suit_to_string(turn_card->suit));
            }
            thrown_cards[turn_idx] = turn_card;

            g->players[turn_idx].card_count--;
            turn_idx = (turn_idx + 1) % g->player_count;
        }
        thrown_totval = calculate_pass_value(thrown_cards, g->player_count) + extra_point;
        pass_winner_id = net_get_outcome(g);

        printf("--- Pass goes to %s with %d of %s, got %d",
                g->players[pass_winner_id].name,
                thrown_cards[pass_winner_id]->value + 1,
                suit_to_string(thrown_cards[pass_winner_id]->suit),
                thrown_totval / 3);
        if (thrown_totval % 3) printf(".%d", (thrown_totval % 3) * 33);
        printf(" points ---\n");

        turn_idx = pass_winner_id;
        cards_remaining -= g->player_count;
        g->pass++;
    }
    // round over

    assert(net_get_leaderboard(g) != -1);

    free(thrown_cards);
    free(selector_arr);

    // TODO: invalid move handling
    return 0;
}
bool client_end_game(struct Game_client *game) {
    struct Player_client *winner = &game->players[net_get_coronation(game)];
    printf("---- GAME OVER: %s won with %d points ----\n", winner->name, winner->game_score);

    close(game->socket);

    char answ;
    while (true) {
        printf("Do you want to reconnect for another game? [y/n]: ");
        flush_istream(fileno(stdin));
        answ = fgetc(stdin);
        switch (answ) {
            case 'Y': case 'y':
                return true;
            case 'N': case 'n':
                return false;
            default:
                printf("Invalid answer.\n");
                continue;
        }
    }
}

bool is_game_over_client(struct Game_client *game) {
    for (int i = 0; i < game->player_count; i++)
        if (game->players[i].game_score >= game->target_score)
            return true;
    return false;
}

int main(int argc, char **argv) {
    
    char *servaddr = "cc.dieg.one";
    char *servport = NULL;
    if (argc >= 2) {
        servaddr = argv[1];
    }
    if (argc >= 3) {
        servport = argv[2];
    }

    while (true) {
        struct Game_client game = { .round = 0 };
        game.socket = client_connect(servaddr, servport);
        assert(game.socket != -1);

        client_setup_game(&game);

        while (is_game_over_client(&game) == false) {
            client_play_round(&game);
            game.round++;
        }

        if (client_end_game(&game) == true) 
            continue;
        else 
            break;
    }
}