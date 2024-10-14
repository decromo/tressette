#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include "client.h"
#include "network.h"

int net_contact_server(struct Game_client *g, enum Response_kind rs_k, void *rs_args);

#endif // CLIENT_NETWORK_H
