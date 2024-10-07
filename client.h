#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

struct Player_client {
    int id;
    char name[PLAYERNAME_STRLEN];
    int game_score;
    int round_score_thirds; // one point counts for three thirds
    int card_count;
    llist hand;
};

struct Game_client {
    int player_count;
    bool team_game;
    int target_score;
    int round;
    int pass;
    int my_id;
    struct Player_client players[4];
    struct Card deck[40];
    struct PQueue queue;
};


#endif // CLIENT_H