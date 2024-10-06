#ifndef SERVER_H
#define SERVER_H

#ifdef __MINGW32__
    #include "windef.h"
#else
    #include <netinet/in.h>
#endif // __MINGW32__

#include "common.h"

struct Player_net {
    int sock_fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    llist pk_q;
};

struct Player_serv {
    int id;
    char name[PLAYERNAME_STRLEN + 1];
    int game_score;
    int round_score_thirds; // one point counts for three thirds
    int card_count;
    llist hand;
    struct Player_net netinfo;
};

struct Game_serv {
    int player_count;
    bool team_game;
    int target_score;
    int round;
    int pass;
    int turn;
    struct Card deck[40];
    struct Player_serv players[4];
};

void give_cards(struct Player_serv *pl, int pl_count, struct Card *deck);
int select_pass_winner(struct Card **thrown, int n_thrown, int first_player_id);
int net_send_playerinfo(Game_serv *g, u8 player_id);
bool is_game_over_serv(struct Player_serv *ps, int n_players, int target);

int net_get_playermove(struct Player_serv *p, int round, int pass);
int net_send_gamestatus(struct Player_serv *ps, int np, int *whom, struct Game_serv *game);
int net_send_playerinfo(struct Player_serv *p);
int net_send_hand(struct Player_serv *ps, int np, int *whom, int n_round);
int net_send_moveresult(struct Player_serv *ps, int np, int *whom, struct Packet_moveresult args);
int net_send_outcome(struct Player_serv *ps, int np, int *whom, struct Packet_outcome args);
int net_send_leaderboard(struct Player_serv *ps, int np, int *whom, struct Packet_leaderboard args);
int net_send_coronation(struct Player_serv *ps, int np, int *whom, int winner_id);

#endif // SERVER_H