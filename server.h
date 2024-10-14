#ifndef SERVER_H
#define SERVER_H

#ifdef __MINGW32__
#include "windef.h"
#else
#include <netinet/in.h>
#endif // __MINGW32__

#include <stdbool.h>
#include "common.h"

struct Game_serv {
    /* const */ int listen_sock;
    /* const */ int player_count;
    /* const */ bool team_game;
    /* const */ int target_score;
    /* const */ struct Card deck[40];
    int round;
    int pass;
    struct Card *pass_cards[4];
    enum Suits pass_suit;
    int pass_master_idx;
    int turn_idx;
    int turn_counter;
    struct Player players[4];
    struct Player *disconnected_players[4];
    int disconnected_player_count;
};

int serv_setup_game(struct Game_serv *g, int listen_sock);
int serv_listen(char *port);
int main(int argc, char **argv);
void give_cards(struct Player ps[2], int n_pl, struct Card *deck);
int serv_simulate_turn(struct Game_serv *g, struct Player *turn_player);
int serv_simulate_pass(struct Game_serv *g, bool is_last_pass);
int select_pass_winner(struct Card **thrown, int n_thrown, int pass_master_idx);
int serv_simulate_round(struct Game_serv *g);
bool is_game_over_serv(struct Player ps[1], int n_players, int target);
void serv_end_game(struct Game_serv *g);
struct Card_node *serv_get_playermove(struct Game_serv *g, struct Player *p, int of_round, int of_pass);
int serv_get_playername(struct Game_serv *g, struct Player *p, int maxsize);

#endif // SERVER_H