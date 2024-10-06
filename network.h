#include <stdint.h>
#include <sys/types.h>
#include "common.h"

#define PORT_DEFAULT 3000
#define PACKET_SIZE 128
#define LOOKFOR_MAXPACKETS 16

    // u8 msg_size;    // size 255 reserved for "message is too big" error 
typedef uint8_t u8;
typedef uint16_t u16;

typedef u8 pk_size_t;
struct Game_status;
struct Packet_hand;

enum __attribute__((__packed__)) Packet_kind {
    SERVER_PKT, CLIENT_PKT
};
enum __attribute__((__packed__)) Request_kind {
    PNAME_RQ, PNAME_AGAIN, MOVE_RQ, MOVE_AGAIN, NO_RQ
};
enum __attribute__((__packed__)) Response_kind {
    PNAME_RS, PMOVE_RS, UPDATEME_RS
};
enum __attribute__((__packed__)) Event_kind {
    WELCOME_CLIENT, GAME_START, PLAYED_CARD, PASS_OVER, ROUND_OVER, GAME_OVER, NO_EVENT
};

struct Game_status {
    u8 player_count;
    u8 is_team_game;
    u8 target_score;
    u8 round;
    u8 pass     : 7;
    u8 turn     : 2;
    char names[4][PLAYERNAME_STRLEN + 1];
    u8 game_scores[4];
    u8 round_score_thirds[4];
} __attribute__((__packed__));
struct Packet_card {
    u8 id;
    u8 suit;
    u8 val;
} __attribute__((__packed__));
struct Packet_hand {
    u8 round;
    u8 n_cards;
    struct Packet_card cards[20];
} __attribute__((__packed__));
struct Server_packet {
    enum Request_kind rq_kind;
    enum Event_kind ev_kind;
    struct Game_status status;
    struct Packet_hand hand;
    char ev_size;
    char ev_data[];
} __attribute__((__packed__));
struct Client_packet {
    enum Response_kind rs_kind;
    pk_size_t rs_size;
    char rs_data[];
} __attribute__((__packed__));
struct Packet {
    enum Packet_kind pk_kind;
    char data[PACKET_SIZE - 1];
} __attribute__((__packed__));

struct RS_packet_name {
    char name[16];
} __attribute__((__packed__));
struct RS_packet_move {
    u8 round;
    u8 pass;
    u8 card_id;
} __attribute__((__packed__));

struct EV_packet_playerinfo {
    u8 id;
} __attribute__((__packed__));
struct EV_packet_playedcard {
    u8 whose;
    struct Packet_card card;    // this struct's id field is meaningless
} __attribute__((__packed__));
struct EV_packet_passover {
    u8 winner_id;
    u8 point_thirds_won;
} __attribute__((__packed__));
struct EV_packet_roundover {
    u8 round;
    u8 score_deltas[4];
} __attribute__((__packed__));
struct EV_packet_gameover {
    u8 winner_id;
} __attribute__((__packed__));

static inline size_t event_sizeof(enum Event_kind k) {
    switch (k) {
        case WELCOME_CLIENT: return sizeof(struct EV_packet_playerinfo);
        case GAME_START: return 0;
        case PLAYED_CARD: return sizeof(struct EV_packet_playedcard);
        case PASS_OVER: return sizeof(struct EV_packet_passover);
        case ROUND_OVER: return sizeof(struct EV_packet_roundover);
        case GAME_OVER: return sizeof(struct EV_packet_gameover);
        case NO_EVENT: return 0;
    }
}

struct PNode {
    struct llist_node node;
    struct Packet *pk;
};

struct Packet *net_recv_packet(int sock);
int net_send_packet(int sock, struct Packet *packet);
struct PNode *net_need_packetkind(enum Packet_kind kind, llist *q, int n, bool wait, int sock);

// struct Packet_reconnect_request {

// }

/*
     server               client
        |    listen ->       l
        |                    l game
        |    <- connect      l
        |    <- name(e)      l
        |     id(e) ->       l
        |   gameinfo(a) ->   l
        |                    l   round
        |     hand(e) ->     l
        |                    l     pass
        |                    l       turn
        |     <- move(i)     l
        |  moveresult(a) ->  l
        |                    l       /turn
        |    outcome(a) ->   l
        |                    l     /pass
        |     score(a) ->    l
        |                    l   /round
        |   endscore(a) ->   l
        |                    l /game

*/