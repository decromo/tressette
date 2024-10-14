#ifndef CLIENT_H
#define CLIENT_H


#include "network.h"
#include "common.h"

struct Game_client {
    /* const */ int player_count;
    /* const */ bool is_team_game;
    /* const */ int target_score;
    /* const */ struct Card deck[40];
    int round;
    int pass;
    struct Card *pass_cards[4];
    enum Suits pass_suit;
    int pass_master_idx;
    int turn_idx;
    int turn_counter;
    char names[4][PLAYERNAME_STRLEN + 1];
    int game_scores[4];
    int round_score_thirds[4];
    struct Player player;
    int hand_selectors[20];
};

void client_round_over(struct Game_client *g, struct EV_packet_roundover *evp, char (*names)[4][PLAYERNAME_STRLEN+1]);
int client_handle_packets(struct Game_client *g);
void client_apply_state(struct Game_client *g, struct Game_status *s, struct Packet_hand *hand);
void game_organize_hand(struct llist *hand, int selector_arr[20], bool print);
void game_print_roundpass(int round, int pass);
void client_setup_game(struct Game_client *g, int servsock);
int client_connect(struct Player_netinfo *netinfo, char *addr, char *port);
int client_prompt_name(char *name, int maxlen);
int client_prompt_move(struct Player *p, enum Suits main_suit);
int sort_card_packets(const void *c1, const void *c2);
void client_game_over(struct Game_client *g, struct EV_packet_gameover *evp,
    char (*names)[4][PLAYERNAME_STRLEN + 1], unsigned char (*scores)[4]);
void client_round_over(struct Game_client *g, struct EV_packet_roundover *evp,
    char (*names)[4][PLAYERNAME_STRLEN + 1]);
void client_pass_over(struct Game_client *g, struct EV_packet_passover *evp,
    char (*names)[4][PLAYERNAME_STRLEN + 1]);
void client_turn_start(struct Game_client *g, struct EV_packet_turnstart *evp,
    char (*names)[4][PLAYERNAME_STRLEN + 1]);
void client_played_card(struct Game_client *g, struct EV_packet_playedcard *evp,
    char (*names)[4][PLAYERNAME_STRLEN + 1]);
bool client_end_game(struct Game_client *game);
bool client_try_reconnect(struct Game_client *g, char *addr, char *port);

#endif // CLIENT_H