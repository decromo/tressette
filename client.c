#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __MINGW32__
#   include "windef.h"
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#endif  // __MINGW32__

#include "client.h"
#include "client_network.h"
#include "common.h"
#include "network.h"
#include "threads.h"

struct RS_packet_move last_RS_move = { 0 };
struct RS_packet_name last_RS_name = { 0 };

// void dbg_selection(struct Card_node *head, int n_cards, int *selector_arr,
//                    int selection_id) {
//     struct Card_node *cn = NULL;
//     for (int i = 0; i < n_cards; i++) {
//         printf("Selection %d -> %d: ", i + 1, selector_arr[i]);
//         cn = head;
//         for (int j = 0; j < n_cards; j++) {
//             if (selector_arr[i] == j) {
//                 printf("%d of %s:%d\n", cn->c->value + 1, suit_to_string(cn->c->suit),
//                        cn->c->suit);
//                 if (i == selection_id)
//                     printf("^^^\t this is what you played.\n");
//                 break;
//             }
//             cn = (struct Card_node *)cn->node.next;
//         }
//     }
// }

int sort_card_packets(const void *c1, const void *c2) {
    return ((struct Packet_card *)c1)->val - ((struct Packet_card *)c2)->val;
}

int client_prompt_name(char *name, int maxlen) {
    size_t bufsiz = maxlen + 2;
    char *buf = calloc(bufsiz, sizeof(char));
    int ret;

    printf("What would you like to call yourself? (max %d characters) ", maxlen);
    flush_instream(fileno(stdin));
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
int client_prompt_move(struct Player *p, enum Suits main_suit) {
    printf("Which card do you want to play? ");
    int selection = -1;
    size_t bufsiz = 8;
    char *buf_str = calloc(bufsiz, sizeof(char));
    char *start_ptr, *end_ptr;
    while (true) {
        flush_instream(fileno(stdin));
        getline(&buf_str, &bufsiz, stdin);
        buf_str[2] = '\0';
        start_ptr = buf_str;
        do {
            selection = strtol(start_ptr, &end_ptr, 10);
        } while (end_ptr == start_ptr++);

        if (selection > 0 && selection <= p->card_count) { break; }

        printf("\nInvalid input, retry: ");
        fflush(stdout);
        continue;
    }
    free(buf_str);

    // assert(selection >= 0 && selection < p->card_count);
    return selection - 1;
}

int client_connect(struct Player_netinfo *netinfo, char *addr, char *port) {
    int sockfd = -1, res = 0, one = 1;
    struct addrinfo hints, *ai_res, *ain;
    assert(addr != NULL);
    assert(port != NULL);

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if ((res = getaddrinfo(addr, port, &hints, &ai_res)) != 0) {
        fprintf(stderr, "Could not resolve address, %s\n", gai_strerror(res));
        return -1;
    }
    printf("Trying to connect to server at %s:%s   ", addr, port);
    int retries = 0;
    while (true) {
        if (retries % 4 == 0) {
            printf("\b\b\b   \b\b\b");
        } else {
            printf(".");
        }
        // printf("\b\b\b");
        // for (int i = 0; i < 3; i++) {
        //     printf("%s", i < (retries % 4) ? "." : " ");
        // }
        fflush(stdout);
        for (ain = ai_res; ain != NULL; ain = ain->ai_next) {
            if ((res = socket(ain->ai_family, ain->ai_socktype, ain->ai_protocol)) == -1) {
                perror("WARN: socket");
                continue;
            };

            if (setsockopt(res, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) == -1) {
                perror("ERRO: setsockopt"); }

            if (connect(res, ain->ai_addr, ain->ai_addrlen) == 0) {
                // if (fcntl(res, F_SETFL, O_NONBLOCK) == -1) {
                //     perror("ERRO: fcntl");
                // }
                memccpy(&netinfo->addr, ain->ai_addr, 0, sizeof(*ain->ai_addr));
                netinfo->addr_len = ain->ai_addrlen;
                sockfd = res;
                break;
            }

            // if (shutdown(res, SHUT_RDWR) == -1) { perror("WARN: shutdown"); }
            close(res);
            res = -1;
            break;
        }

        // Connection successful
        if (ain != NULL && res != -1) {
            printf("\nConnection successful!\n");
            break;
        }

        // Connection unsuccessful, retry in 1 second.
        retries++;
        sleep(1);
    }

    freeaddrinfo(ai_res);
    return sockfd;
}

void client_setup_game(struct Game_client *g, int servsock) {

    initialize_deck(g->deck);

    g->player.id = -1;  // waiting for this to get set by a welcome event packet

    llist_init(&g->player.hand);

    thread_recv_init(&g->player.netinfo.pk_queue, servsock);
    // FIXME
    // printf("Waiting for acknowledgement...");
    // fflush(stdout);
    // net_get_playerinfo(game);
    // printf(" acknowledged, your id is %d.\nWaiting for the game to start...",
    //        game->my_id);
    // fflush(stdout);
    // net_get_gameinfo(game);
    // printf(" game is starting!\n");
}

void game_print_roundpass(int round, int pass) {
    char extra_dashes[3] = "\0\0\0";
    int dash_i = 0;
    if (round / 10) extra_dashes[dash_i++] = '-';
    if (pass / 10) extra_dashes[dash_i++] = '-';
    printf("\t\t\t\t\t\b\b\b------------------------%s\n", extra_dashes);
    printf("\t\t\t\t\t\b\b\b--- Round %d - Pass %d ---\n", round + 1, pass + 1);
    printf("\t\t\t\t\t\b\b\b------------------------%s\n", extra_dashes);
}
void game_organize_hand(struct llist *hand, int selector_arr[20], bool print) {
    struct Packet_card buf[4][20];
    memset(buf, 0, 80 * sizeof(struct Packet_card));
    int sizes[4] = { 0 };
    int suits_remaining_tot = 4;
    char suits_remaining[5] = "AAAA";

    enum Suits cs = -1;
    struct Card_node *cn = (struct Card_node *)hand->head;
    for (int i = 0; i < hand->size; i++) {
        assert(cn != NULL);
        cs = cn->c->suit;
        buf[cs][sizes[cs]].id = i;
        buf[cs][sizes[cs]].suit = cs;
        buf[cs][sizes[cs]].val = cn->c->value;
        sizes[cs]++;
        cn = (struct Card_node *)cn->node.next;
    }
    for (int i = 0; i < 4; i++) { qsort(buf[i], sizes[i], sizeof(struct Packet_card), sort_card_packets); }

    int i_card = 0;
    int line = 0;
    char selector_str[16] = "";
    bool with_selection = (selector_arr != NULL);

    #define PRINT(...) \
        if (print == true) printf(__VA_ARGS__);

    PRINT(".\t.\t.\t.\t.\t.   Your Hand    .\t.\t.\t.\t.\t.\n\n");
    PRINT("\tDenara\t\t\tCoppe\t\t\tBastoni\t\t\tSpade\n");

    while (memcmp(suits_remaining, "\0\0\0\0", 4) != 0) {
        for (int i_suit = 0; i_suit < 4; i_suit++) {
            if (line < sizes[i_suit]) {
                if (with_selection == true) {
                    snprintf(selector_str, 15, "\b\b\b(%s%d) ", 
                        (i_card < 9) ? " " : "", 
                        i_card + 1);
                    selector_arr[i_card] = buf[i_suit][line].id;
                    i_card++;
                }
                PRINT("\t%s%d of %s\t",
                    selector_str,
                    buf[i_suit][line].val + 1,
                    suit_to_string(buf[i_suit][line].suit));
            } else {
                PRINT("\t\t%s", with_selection ? "\t" : "");
                suits_remaining[i_suit] = '\0';
            }
        }
        PRINT("\n");
        line++;
    }
    #undef PRINT
}

void client_apply_state(struct Game_client *g, struct Game_status *s, struct Packet_hand *hand) {
    g->player_count = s->player_count;
    g->is_team_game = s->is_team_game;
    g->target_score = s->target_score;
    g->round = s->round;
    g->pass = s->pass;
    g->pass_master = s->pass_master;
    g->pass_suit = s->pass_suit;
    g->turn_idx = s->turn_idx;
    g->turn_counter = s->turn_counter;
    for (int i = 0; i < 4; i++) {
        g->pass_cards[i] = &g->deck[s->pass_cards[i].suit * 10 + s->pass_cards[i].val];
        strncpy(g->names[i], s->names[i], PLAYERNAME_STRLEN);
        g->game_scores[i] = s->game_scores[i];
        g->round_score_thirds[i] = s->round_score_thirds[i];
    }

    g->player.card_count = hand->n_cards;
    g->player.game_score = s->game_scores[g->player.id];
    g->player.round_score_thirds = s->round_score_thirds[g->player.id];

    llist_nuke(&g->player.hand, NULL);  // is this fine without a fun?
    struct Card_node *cnp;
    for (int i = 0; i < hand->n_cards; i++) {
        cnp = malloc(sizeof(struct Card_node));
        assert(cnp != NULL);

        cnp->c = &g->deck[hand->cards[i].suit * 10 + hand->cards[i].val];
        llist_append(&g->player.hand, (llist_node *)cnp);
    }
}
int client_handle_packets(struct Game_client *g) {
    struct Packet *packet = NULL;
    struct Server_packet *sp = NULL;
    bool game_over = false;

    while (game_over == false) {
        // check if the connection was closed
        if (g->player.netinfo.pk_queue.closed == true) {
            return -1;
        }

        // get a packet from the recv thread and free its node
        packet = pop_packet(&g->player.netinfo.pk_queue);

        if (packet == NULL) {
            usleep(300);
            continue;
        }

        // only read server packets
        if (packet->pk_kind != SERVER_PKT) {
            free(packet);
            continue;
        }

        sp = (struct Server_packet *)packet->data;

        client_apply_state(g, &sp->status, &sp->hand);

        switch (sp->rq_kind) {
        case RQ_NAME_INVALID:
        case RQ_NAME:
            last_RS_name.name_len = client_prompt_name(last_RS_name.name, PLAYERNAME_STRLEN);
        case RQ_NAME_AGAIN:
            net_contact_server(g, RS_NAME, &last_RS_name);
            break;

        case RQ_MOVE_INVALID:
            game_organize_hand(&g->player.hand, g->hand_selectors, true);
            printf("Your last move was invalid, try again.\n");
        case RQ_MOVE:
            game_organize_hand(&g->player.hand, g->hand_selectors, false);
            last_RS_move.round = g->round;
            last_RS_move.pass = g->pass;
            last_RS_move.card_id = g->hand_selectors[client_prompt_move(&g->player, g->pass_suit)];
        case RQ_MOVE_AGAIN:
            net_contact_server(g, RS_MOVE, &last_RS_move);
            break;

        case RQ_NONE:
            break;
        }

        switch (sp->ev_kind) {
        case EV_WELCOME:
            g->player.id = ((struct EV_packet_welcome *)sp->ev_data)->id;
            printf("Acknowledged by server, your id is %d.\n", g->player.id);
            break;
        case EV_GAME_START:
            printf("Game is starting !!\n");
            break;
        case EV_GAME_OVER:
            client_game_over(g, (struct EV_packet_gameover *)sp->ev_data, 
                &sp->status.names, &sp->status.game_scores);
            game_over = true;
            break;
        case EV_ROUND_START:
            printf("Cards are given.\n");
            break;
        case EV_ROUND_OVER:
            client_round_over(g, (struct EV_packet_roundover *)sp->ev_data, &sp->status.names);
            break;
        case EV_PASS_START:
            game_print_roundpass(g->round, g->pass);
            game_organize_hand(&g->player.hand, g->hand_selectors, true);
            break;
        case EV_PASS_OVER:
            client_pass_over(g, (struct EV_packet_passover *)sp->ev_data, &sp->status.names);
            break;
        case EV_TURN_START:
            client_turn_start(g, (struct EV_packet_turnstart *)sp->ev_data, &sp->status.names);
            break;
        case EV_PLAYED_CARD:
            client_played_card(g, (struct EV_packet_playedcard *)sp->ev_data, &sp->status.names);
        case EV_NONE:
            break;
        }

        free(packet);
    }
    return 0;
}

// depends on g.player_count; TODO: call end game routine??
void client_game_over(struct Game_client *g, struct EV_packet_gameover *evp, char (*names)[4][PLAYERNAME_STRLEN+1], unsigned char (*scores)[4]) {
    printf("** GAME OVER: %s won with %d points. **\n", *names[evp->winner_id], *scores[evp->winner_id]);
    printf("----- final scores -----\n");
    for (int i = 0; i < g->player_count; i++) {
        printf("| %s with %d points \t\t|\n", *names[evp->winner_id], *scores[evp->winner_id]);
    }
    printf("------------------------\n");
}
// depends on g.player_count
void client_round_over(struct Game_client *g, struct EV_packet_roundover *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    printf("--- round %d net gains ---\n", evp->round);
    for (int i = 0; i < g->player_count; i++) {
        printf("| %s gained %d points \t\t|\n", *names[i], evp->score_deltas[i]);
    }
    printf("-------------------------\n");
}
// depends on g.pass_cards
void client_pass_over(struct Game_client *g, struct EV_packet_passover *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    if (g->player.id == evp->winner_id) {
        printf("--- You won this pass with ");
    } else {
        printf("--- Pass goes to %s with ", *names[evp->winner_id]);
    }
    printf("%d of %s, got %d",
        g->pass_cards[evp->winner_id]->value + 1,
        suit_to_string(g->pass_cards[evp->winner_id]->suit),
        evp->point_thirds_won / 3);
    if (evp->point_thirds_won % 3 != 0)
        printf(".%d", (evp->point_thirds_won % 3) * 33);
    printf(" points ---\n");
}
// organizes hand selectors
void client_turn_start(struct Game_client *g, struct EV_packet_turnstart *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    if (evp->who == g->player.id) {
        // TODO: make better prompt
        printf("It's your turn\n");
    } else {
        printf("It's %s's turn.\n", *names[evp->who]);
    }
}
// depends on g.turn_counter: needs to be the turn where the card was played
void client_played_card(struct Game_client *g, struct EV_packet_playedcard *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    if (evp->whose == g->player.id) {
        printf("You played ");
    } else {
        printf("%s played ", *names[evp->whose]);
    }
    printf("%d of %s.\n", evp->card.val + 1, suit_to_string(evp->card.suit));
    if (g->turn_counter == 0) {
        printf("This pass will be played on %s.\n", suit_to_string(evp->card.suit));
    }
}

int main(int argc, char **argv) {
    char *servaddr = "cc.dieg.one";
    char servport[8];
    snprintf(servport, 5, "%d", PORT_DEFAULT);
    if (argc >= 2) { servaddr = argv[1]; }
    if (argc >= 3) { strncpy(servport, argv[2], 5); }

    while (true) {
        int servsock = -1;
        struct Game_client game = { 0 };

        servsock = client_connect(&game.player.netinfo, servaddr, servport);
        assert(servsock != -1);

        client_setup_game(&game, servsock);

        while (client_handle_packets(&game) == -1) {
            // lost connection
            if (client_try_reconnect(&game, servaddr, servport) == true)
                continue;
            else
                return 1;
        }

        if (client_end_game(&game) == true)
            continue;
        else
            break;
    }

    printf("Thanks for playing ~~~!");
    return 0;
}

bool client_end_game(struct Game_client *g) {
    char answ;
    while (true) {
        printf("Do you want to reconnect for another game? [y/n]: ");
        flush_instream(fileno(stdin));
        answ = fgetc(stdin);
        switch (answ) {
        case 'Y':
        case 'y':
            return true;
        case 'N':
        case 'n':
            return false;
        default:
            printf("Invalid answer.\n");
            continue;
        }
    }
}

bool client_try_reconnect(struct Game_client *g, char *addr, char *port) {
    char answ;
    while (true) {
        printf("Connection lost, do you want to try reconnecting? [y/n]: ");
        flush_instream(fileno(stdin));
        answ = fgetc(stdin);
        switch (answ) {
        case 'Y':
        case 'y':
            break;
        case 'N':
        case 'n':
            return false;
        default:
            printf("Invalid answer.\n");
            continue;
        }

        int servsock = client_connect(&g->player.netinfo, addr, port);
        if (servsock == -1) { return false; }

        client_setup_game(g, servsock);

        return true;
    }
}