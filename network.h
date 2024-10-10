#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <sys/types.h>
#ifndef COMMON_H
#   include "common.h"
#endif

#define PORT_DEFAULT 3000
#define PACKET_SIZE 192
#define LOOKFOR_MAXPACKETS 16

// u8 msg_size;    // size 255 reserved for "message is too big" error 
typedef uint8_t u8;
typedef uint16_t u16;

typedef u8 pk_size_t;

struct Game_status;
struct Packet_hand;
struct Packet_card;

enum __attribute__((__packed__)) Packet_kind {
    SERVER_PKT, CLIENT_PKT
};
enum __attribute__((__packed__)) Request_kind {
    RQ_NAME, RQ_NAME_AGAIN, RQ_NAME_INVALID, RQ_MOVE, RQ_MOVE_AGAIN, RQ_MOVE_INVALID, RQ_NONE
};
enum __attribute__((__packed__)) Response_kind {
    RS_NAME, RS_MOVE, RS_UPDATEME
};
enum __attribute__((__packed__)) Event_kind {
    EV_WELCOME, EV_GAME_START, EV_GAME_OVER, EV_ROUND_START, EV_ROUND_OVER, EV_PASS_START,
        EV_PASS_OVER, EV_TURN_START, EV_PLAYED_CARD, EV_NONE
};

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

struct Game_status {
    u8 player_count;
    u8 is_team_game;
    u8 target_score;
    u8 round;
    u8 pass;
    struct Packet_card pass_cards[4];
    u8 pass_master : 2;
    u8 pass_suit : 2;
    u8 turn_idx : 2;
    u8 turn_counter : 2;
    char names[4][PLAYERNAME_STRLEN + 1];
    u8 game_scores[4];
    u8 round_score_thirds[4];
} __attribute__((__packed__));

struct Server_packet {
    enum Request_kind rq_kind;
    enum Event_kind ev_kind;
    struct Game_status status;
    struct Packet_hand hand;
    pk_size_t ev_size;
    char ev_data[];
} __attribute__((__packed__));
struct Client_packet {
    enum Response_kind rs_kind;
    pk_size_t rs_size;
    char rs_data[];
} __attribute__((__packed__));
struct Packet {
    enum Packet_kind pk_kind;
    char data[PACKET_SIZE - sizeof(enum Packet_kind)];
} __attribute__((__packed__));

struct RS_packet_name {
    u8 name_len;
    char name[16];
} __attribute__((__packed__));
struct RS_packet_move {
    u8 round;
    u8 pass;
    u8 card_id;
} __attribute__((__packed__));

struct EV_packet_welcome {
    u8 id;
} __attribute__((__packed__));
struct EV_packet_unknownreconnection {
    // TODO
} __attribute__((__packed__));
struct EV_packet_playedcard {
    u8 whose;
    struct Packet_card card;    // this struct's id field is meaningless
} __attribute__((__packed__));
struct EV_packet_passover {
    u8 winner_id;
    u8 point_thirds_won;
} __attribute__((__packed__));
struct EV_packet_turnstart {
    u8 who;
} __attribute__((__packed__));
struct EV_packet_roundover {
    u8 round;
    u8 score_deltas[4];
} __attribute__((__packed__));
struct EV_packet_gameover {
    u8 winner_id;
} __attribute__((__packed__));


// EV_WELCOME_CLIENT, EV_GAME_START, EV_PASS_START, PLAYED_CARD, EV_PASS_OVER, EV_ROUND_OVER, EV_GAME_OVER, EV_NONE
static inline pk_size_t event_sizeof(enum Event_kind k) {
    switch (k) {
    case EV_WELCOME: return sizeof(struct EV_packet_welcome);
    case EV_GAME_START: return 0;
    case EV_GAME_OVER: return sizeof(struct EV_packet_gameover);
    case EV_ROUND_START: return 0;
    case EV_ROUND_OVER: return sizeof(struct EV_packet_roundover);
    case EV_PASS_START: return 0;
    case EV_PASS_OVER: return sizeof(struct EV_packet_passover);
    case EV_TURN_START: return sizeof(struct EV_packet_turnstart);
    case EV_PLAYED_CARD: return sizeof(struct EV_packet_playedcard);
    case EV_NONE: return 0;
    }
}
static inline const char *event_nameof(enum Event_kind k) {
    switch (k) {
    case EV_WELCOME: return "welcome";
    case EV_GAME_START: return "gamestart";
    case EV_GAME_OVER: return "gameover";
    case EV_PASS_START: return "passstart";
    case EV_PASS_OVER: return "passover";
    case EV_ROUND_START: return "roundstart";
    case EV_ROUND_OVER: return "roundover";
    case EV_TURN_START: return "turnstart";
    case EV_PLAYED_CARD: return "playedcard";
    case EV_NONE: return "noevent";
    }
}

// RQ_NAME, RQ_NAME_AGAIN, RQ_NAME_INVALID, RQ_MOVE, RQ_MOVE_AGAIN, RQ_MOVE_INVALID, RQ_NONE
static inline const char *request_nameof(enum Request_kind k) {
    switch (k) {
    case RQ_NAME: return "name";
    case RQ_NAME_AGAIN: return "name_again";
    case RQ_NAME_INVALID: return "name_invalid";
    case RQ_MOVE: return "move";
    case RQ_MOVE_AGAIN: return "move_again";
    case RQ_MOVE_INVALID: return "move_invalid";
    case RQ_NONE: return "norequest";
    }
}

// RS_NAME, RS_MOVE, RS_UPDATEME
static inline pk_size_t response_sizeof(enum Response_kind k) {
    switch (k) {
    case RS_NAME: return sizeof(struct RS_packet_name);
    case RS_MOVE: return sizeof(struct RS_packet_move);
    case RS_UPDATEME: return 0;
    }
}

int net_send_packet(int sock, struct Packet *packet);
struct Packet *net_recv_packet(int sock);
void free_pnode_packet(void *arg);

// struct Packet_reconnect_request {

// }

#endif // NETWORK_H