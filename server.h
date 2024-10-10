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
    int pass_master;
    int turn_idx;
    int turn_counter;
    struct Player players[4];
    struct Player *disconnected_players[4];
};

void give_cards(struct Player *pl, int pl_count, struct Card *deck);
int select_pass_winner(struct Card **thrown, int n_thrown, int first_player_id);
int net_get_playermove(struct Player *p, int of_round, int of_pass);
bool is_game_over_serv(struct Player *ps, int n_players, int target);
void serv_shutdown_game(struct Game_serv *g);


#endif // SERVER_H