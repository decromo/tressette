#include <stdint.h>
#include <sys/types.h>
#include "common.h"

#define PORT_DEFAULT 3000
#define PACKET_SIZE 128
#define LOOKFOR_MAXPACKETS 16

typedef uint8_t u8;
typedef uint16_t u16;

enum __attribute__((__packed__)) Packet_kind {
    SERVER_PKT, CLIENT_PKT
};
enum __attribute__((__packed__)) Request_kind {
    PNAME_RQ, PNAME_RETRY, MOVE_RQ, MOVE_RETRY, NO_RQ
};
enum __attribute__((__packed__)) Response_kind {
    PNAME_RS, MOVE_RS, UPDATEME_RS
};
enum __attribute__((__packed__)) Event_kind {
    GAME_START, PLAYED_CARD, PASS_OVER, ROUND_OVER, GAME_OVER
};
struct Packet {
    enum Packet_kind pk_kind;
    char data[PACKET_SIZE - 1];
} __attribute__((__packed__));
    u8 msg_size;    // size 255 reserved for "message is too big" error 
struct Packet_response {

}
struct Packet_name {
    u8 size;
    char name[PLAYERNAME_STRLEN];
} __attribute__((__packed__));
struct Packet_playerinfo {
    u8 id;
    struct Packet_name name;
} __attribute__((__packed__));
struct Packet_gameinfo {
    u8 round;
    u8 player_count;
    u8 team_game;
    u8 target_score;
    struct Packet_playerinfo players[4];
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
struct Packet_move {
    u8 round;
    u8 pass;
    u8 card_id;
} __attribute__((__packed__));
struct Packet_moveresult {
    u8 round;
    u8 pass;
    u8 player_id;
    struct Packet_card card;
} __attribute__((__packed__));
struct Packet_outcome {
    u8 round;
    u8 pass;
    u8 winner_id;
    u8 n_scores;
    u8 scores[4];
} __attribute__((__packed__));
struct Packet_leaderboard {
    u8 round;
    u8 n_players;
    u8 scores[4];
} __attribute__((__packed__));
struct Packet_coronation {
    u8 winner_id;
} __attribute__((__packed__));

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