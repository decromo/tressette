#ifndef CLIENT_H
#define CLIENT_H

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
    int pass_master;
    int turn_idx;
    int turn_counter;
    char names[4][PLAYERNAME_STRLEN + 1];
    int game_scores[4];
    int round_score_thirds[4];
    struct Player player;
    int hand_selectors[20];
};


#endif // CLIENT_H