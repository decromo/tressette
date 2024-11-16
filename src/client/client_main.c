#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/platform.h"

#include <raylib.h>

#include "client.h"
#include "client_network.h"

#include "client_seco.h"
#include "scenes/scenes.h"

#include "../common/common.h"
#include "../common/network.h"
#include "../common/threads.h"

struct RS_packet_move last_RS_move = { 0 };
struct RS_packet_name last_RS_name = { 0 };

struct addrinfo *client_addrinfo(char *addr, char *port) {
    assert(addr != NULL);
    assert(port != NULL);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_protocol = 0;

    struct addrinfo *ai_res;
    int res = 0;

    res = getaddrinfo(addr, port, &hints, &ai_res);

    if (res != 0) {
        fprintf(stderr, "Could not resolve address, %s\n", gai_strerror(res));
        return NULL;
    }

    return ai_res;
}
int client_connect(struct Player_netinfo *netinfo, struct addrinfo *ai_list) {
    int sockfd = -1, 
        res = 0;

    struct addrinfo *ain;

    {
        static int retries = 0;
        if (retries % 60 == 0) {
            char status[512] = {0};
            sprintf(status, "Trying to connect to server at %s:%d", ai_list->ai_canonname, ntohs(((struct sockaddr_in*)ai_list->ai_addr)->sin_port));
            for (int i = 0; i < (retries / 60) % 4; i++) {
                strcat(status, ".");
            }
            render_status_text(strlen(status), status);
        }
        retries++;
    }

    for (ain = ai_list; ain != NULL; ain = ain->ai_next) {
        res = socket(ain->ai_family, ain->ai_socktype, ain->ai_protocol);
        if (res == -1) {
            perror("WARN: socket");
            continue;
        };
        sockfd = res;

        res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
        if (res == -1) {
            perror("ERRO: setsockopt");
        }

        if (connect(sockfd, ain->ai_addr, ain->ai_addrlen) == 0) {
            // if (fcntl(res, F_SETFL, O_NONBLOCK) == -1) {
            //     perror("ERRO: fcntl");
            // }
            printf("\nConnection successful!\n");

            memccpy(&netinfo->addr, ain->ai_addr, 0, sizeof(*ain->ai_addr));
            netinfo->addr_len = ain->ai_addrlen;

            break;
        }

        // if (shutdown(res, SHUT_RDWR) == -1) { perror("WARN: shutdown"); }
        close(sockfd);
        sockfd = -1;
    }

    // Connection unsuccessful, return -1 and retry later
    if (ain == NULL || sockfd == -1) {
        sockfd = -1;
    }

    return sockfd;
}

void client_setup_game(struct Game_client *g, int servsock) {

    initialize_deck(g->deck);

    g->player.id = -1;  // waiting for this to get set by a welcome event packet

    llist_init(&g->player.hand);

    thread_recv_init(&g->player.netinfo.pk_queue, servsock);
}

int sort_card_packets(const void *c1, const void *c2) {
    return ((struct Packet_card *)c1)->val - ((struct Packet_card *)c2)->val;
}
void game_organize_hand(struct llist *hand, int selector_arr[20], bool print) {
    struct Packet_card buf[4][20];
    memset(buf, 0, 80 * sizeof(struct Packet_card));
    int sizes[4] = { 0 };
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
    char selector_str[20] = "";
    bool with_selection = (selector_arr != NULL);

    #define PRINT(...) \
        if (print == true) printf(__VA_ARGS__);

    PRINT(".\t.\t.\t.\t.\t.   Your Hand    .\t.\t.\t.\t.\t.\n\n");
    PRINT("\tDenari\t\t\tCoppe\t\t\tSpade\t\t\tBastoni\n");

    while (memcmp(suits_remaining, "\0\0\0\0", 4) != 0) {
        for (int i_suit = 0; i_suit < 4; i_suit++) {
            if (line < sizes[i_suit]) {
                if (with_selection == true) {
                    snprintf(selector_str, 18, "\b\b\b(%s%d) ", 
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
void game_print_roundpass(int round, int pass) {
    char extra_dashes[3] = "\0\0\0";
    int dash_i = 0;
    if (round / 10) extra_dashes[dash_i++] = '-';
    if (pass / 10) extra_dashes[dash_i++] = '-';
    printf("\t\t\t\t\t\b\b\b------------------------%s\n", extra_dashes);
    printf("\t\t\t\t\t\b\b\b--- Round %d - Pass %d ---\n", round + 1, pass + 1);
    printf("\t\t\t\t\t\b\b\b------------------------%s\n", extra_dashes);
}

int client_prompt_name(unsigned int maxlen, char *name) {
    size_t bufsiz = maxlen + 2;
    char *buf = calloc(bufsiz, sizeof(char));
    int ret;

    printf("What would you like to call yourself? (max %d characters) ", maxlen);
    fflush(stdout);
    flush_instream(stdin);
    fgets(buf, maxlen+1, stdin);
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
int client_prompt_move(struct Player *p) {
    int selection = -1;
    char buf_str[8] = { 0 };
    char *end_ptr;

    printf("Which card do you want to play? ");
    while (true) {
        fflush(stdout);
        flush_instream(stdin);
        fgets(buf_str, 8, stdin);
        buf_str[7] = '\0';
        selection = strtol(buf_str, &end_ptr, 10);

        if (buf_str != end_ptr 
            && selection > 0 
            && selection <= p->card_count) { break; }

        printf("\nInvalid input, retry: ");
        continue;
    }

    return selection - 1;
}

// depends on g.player_count; TODO: call end game routine??
void CEV_game_over(struct Game_client *g, struct EV_packet_gameover *evp, char (*names)[4][PLAYERNAME_STRLEN+1], unsigned char (*scores)[4]) {
    printf("** GAME OVER: %s won with %d points. **\n", (*names)[evp->winner_id], (*scores)[evp->winner_id]);
    printf("----- final scores -----\n");
    for (int i = 0; i < g->player_count; i++) {
        printf("| %s with %d points \t\t|\n", (*names)[evp->winner_id], (*scores)[evp->winner_id]);
    }
    printf("------------------------\n");
}
// depends on g.player_count
void CEV_round_over(struct Game_client *g, struct EV_packet_roundover *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    printf("--- round %d net gains ---\n", evp->round);
    for (int i = 0; i < g->player_count; i++) {
        printf("| %s gained %d points \t\t|\n", (*names)[i], evp->score_deltas[i]);
    }
    printf("-------------------------\n");
}
// depends on g.pass_cards
void CEV_pass_over(struct Game_client *g, struct EV_packet_passover *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    if (g->player.id == evp->winner_id) {
        printf("--- You won this pass with ");
    } else {
        printf("--- Pass goes to %s with ", (*names)[evp->winner_id]);
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
void CEV_turn_start(struct Game_client *g, struct EV_packet_turnstart *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    if (evp->who == g->player.id) {
        // TODO: make better prompt
        printf("It's your turn\n");
    } else {
        printf("It's %s's turn.\n", (*names)[evp->who]);
    }
}
// depends on g.turn_counter: needs to be the turn where the card was played
void CEV_played_card(struct Game_client *g, struct EV_packet_playedcard *evp, char (*names)[4][PLAYERNAME_STRLEN+1]) {
    char str[1024] = {0};
    if (evp->whose == g->player.id) {
        strncat(str, "You played ", 16);
    } else {
        strncat(str, TextFormat("%s played ", (*names)[evp->whose]), 256);
    }
    strncat(str, TextFormat("%d of %s.\n", evp->card.val + 1, suit_to_string(evp->card.suit)), 256);
    if (g->turn_counter == 0) {
        strncat(str, TextFormat("This pass will be played on %s.\n", suit_to_string(evp->card.suit)), 256);
    }
    printf("%s", str);
    setStatusText(str);
}

void client_apply_state(struct Game_client *g, struct Game_status *s, struct Packet_hand *hand) {
    g->player_count = s->player_count;
    g->is_team_game = s->is_team_game;
    g->target_score = s->target_score;
    g->round = s->round;
    g->pass = s->pass;
    g->pass_master_idx = s->pass_master_idx;
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
    bool is_game_over = false;

    clock_t start_clock = clock();
    int return_after_ms = 5;

    do {
        // check if the connection was closed
        if (g->player.netinfo.pk_queue.closed == true) {
            return -1;
        }

        // get a packet from the recv thread and free its node
        packet = pop_packet(&g->player.netinfo.pk_queue);

        // tick over if no packets were found
        if (packet == NULL) {
            // usleep(500);
            continue;
        }

        // only read server packets
        if (packet->pk_kind != SERVER_PKT) {
            free(packet);
            continue;
        }

        sp = (struct Server_packet *)packet->data;

        client_apply_state(g, &sp->status, &sp->hand);

        switch (sp->ev_kind) {
        case EV_WELCOME:
            g->player.id = ((struct EV_packet_welcome *)sp->ev_data)->id;
            printf("Acknowledged by server, your id is %d.\n", g->player.id);
            break;
        case EV_GAME_START:
            printf("Game is starting !!\n");
            switchScene(TAG_scene_game);
            break;
        case EV_GAME_OVER:
            CEV_game_over(g, (struct EV_packet_gameover *)sp->ev_data, 
                &sp->status.names, &sp->status.game_scores);
            is_game_over = true;
            break;
        case EV_ROUND_START:
            printf("Cards are given.\n");
            break;
        case EV_ROUND_OVER:
            CEV_round_over(g, (struct EV_packet_roundover *)sp->ev_data, &sp->status.names);
            break;
        case EV_PASS_START:
            game_print_roundpass(g->round, g->pass);
            game_organize_hand(&g->player.hand, g->hand_selectors, true);
            break;
        case EV_PASS_OVER:
            CEV_pass_over(g, (struct EV_packet_passover *)sp->ev_data, &sp->status.names);
            break;
        case EV_TURN_START:
            CEV_turn_start(g, (struct EV_packet_turnstart *)sp->ev_data, &sp->status.names);
            break;
        case EV_PLAYED_CARD:
            CEV_played_card(g, (struct EV_packet_playedcard *)sp->ev_data, &sp->status.names);
        case EV_NONE:
            break;
        }

        switch (sp->rq_kind) {
        case RQ_NONE:
            break;
        default:
            assert(client_enqueue_request(sp->rq_kind, g) == 0);
            break;
        }

    } while ( ((clock() - start_clock) * CLOCKS_PER_SEC * 1000) < (return_after_ms) );

    free(packet);
    return is_game_over;
}
int client_enqueue_request(enum Request_kind rq, struct Game_client *g) {
    if (g->rq_queue_size >= 20) return 1;

    g->rq_queue[g->rq_queue_freeIdx] = rq;
    g->rq_queue_size++;
    for (int i = 0; i < 20; i++) {
        if (g->rq_queue[i] == RQ_NONE)  {
            g->rq_queue_freeIdx = i;
            break;
        }
    }
    return 0;
}
int client_handle_requests(struct Game_client *g) {

    // for (int i = g->rq_queue_size - 1; i >= 0; i--) {

    // }
    int seen = 0;
    int handled = 0;
    int first_free = -1;
    int ret = 0;
    for (int i = 0; seen < g->rq_queue_size && i < 20; i--) { // FIXME
        if (g->rq_queue[i] == RQ_NONE) {
            if (first_free == -1) first_free = i;
            continue;
        }

        #define REQUEST_HANLED \
            { \
                handled++; \
                g->rq_queue[i] = RQ_NONE; \
                if (first_free == -1) first_free = i; \
            }

        seen++;
        switch (g->rq_queue[i]) {
        case RQ_NONE:
            break;

        case RQ_NAME_INVALID:
        case RQ_NAME:
            ret = client_prompt_name(PLAYERNAME_STRLEN, last_RS_name.name);
            if (ret == -1) break;
            last_RS_name.name_len = ret;
        case RQ_NAME_AGAIN:
            net_contact_server(g, RS_NAME, &last_RS_name);
            REQUEST_HANLED
            break;

        case RQ_MOVE_INVALID:
            printf("Your last move was invalid, try again.\n");
        case RQ_MOVE:
            setStatusText("Your turn\n");
            ret = scene_game_selectCard(g);
            if (ret == -1) break;
            last_RS_move.round = g->round;
            last_RS_move.pass = g->pass;
            last_RS_move.card_id = ret;
        case RQ_MOVE_AGAIN:
            net_contact_server(g, RS_MOVE, &last_RS_move);
            REQUEST_HANLED
            break;
        }
        i--;
    }
    #undef REQUEST_HANLED

    g->rq_queue_size -= handled;
    if (first_free != -1) g->rq_queue_freeIdx = first_free;

    return 0;
}

bool client_prompt_endgame(void) {
    char answ;
    while (true) {
        printf("Do you want to reconnect for another game? [y/n]: ");
        flush_instream(stdin);
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
bool client_prompt_reconnect(void) {
    char answ;
    while (true) {
        printf("Connection lost, do you want to try reconnecting? [y/n]: ");
        flush_instream(stdin);
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

int client_main(void *game_raw)
{
    struct Game_client *game = game_raw;
    
    if (!game->connection_established) {
        if (!game->addrinfo_found) {
            char *servaddr = "cc.dieg.one";
            char servport[8];
            snprintf(servport, 5, "%d", PORT_DEFAULT);
            // if (argc >= 2) { servaddr = argv[1]; }
            // if (argc >= 3) { strncpy(servport, argv[2], 5); }
            game->serv_ai = client_addrinfo(servaddr, servport);
            if (game->serv_ai == NULL) {
                exit(1);
            }
            game->addrinfo_found = true;
        }
        if (game->addrinfo_found) {
            int servsock = -1;
            servsock = client_connect(&game->player.netinfo, game->serv_ai);

            if (servsock == -1) { goto render; }

            game->connection_established = true;
            game->lost_connection = false;
            client_setup_game(game, servsock);
            game->addrinfo_found = false; // make sure we don't use freed addrinfo
            freeaddrinfo(game->serv_ai);
        }
    }
    switch (client_handle_packets(game)) {
    case 0: // exit switch and continue playing
        break;
    case 1: // game is over
        game->game_over = true;
        break;
    case -1: // lost connection
        game->connection_established = false;
        game->lost_connection = true;
        break;
    default:
        break;
    }

    render:
    render_loop(game);

    client_handle_requests(game);

    // div();

    if (game->lost_connection && !client_prompt_reconnect()) {
        game->game_aborted = true;
        return -1;
    }

    if (game->game_over && !client_prompt_endgame()) {
        return -1;
    }

    if (WindowShouldClose()) {
        return -1;
    }

    return game->game_over;
}




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