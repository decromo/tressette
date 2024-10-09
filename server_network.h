#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

#include "server.h"
#include "network.h"

void net_get_playername(struct Player *p, int maxsize);
struct Packet *net_serv_forge_packet(struct Game_serv *g, struct Player *p);
int net_notify_clients(struct Game_serv *g, int whom[1], int n_whom, enum Request_kind rq_kind, enum Event_kind ev_kind, void *args);
struct PNode *net_serv_need_response(enum Response_kind rs_k, struct PQueue *pk_q, int n, bool wait);
bool handle_disconnections(void *arg);

#endif // SERVER_NETWORK_H