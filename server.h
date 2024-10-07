#ifndef SERVER_H
#define SERVER_H

#ifdef __MINGW32__
    #include "windef.h"
#else
    #include <netinet/in.h>
#endif // __MINGW32__

#include "common.h"

struct Player_net {
    struct PQueue pk_queue;
    struct sockaddr_in addr;
    socklen_t addr_len;
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
bool is_game_over_serv(struct Player_serv *ps, int n_players, int target);

struct Packet *net_serv_forge_packet(struct Game_serv *g, struct Player_serv *p);
int net_notify_clients(struct Game_serv *g, int n_whom, int whom[1], enum Request_kind rq_kind, enum Event_kind ev_kind, void *args);
struct PNode *net_serv_need_response(enum Response_kind rs_k, struct PQueue *pk_q, int n, bool wait);

#endif // SERVER_H