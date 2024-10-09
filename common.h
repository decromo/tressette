#ifndef COMMON_H
#define COMMON_H

#define PLAYERNAME_STRLEN 15
#define DEFAULT_TARGET_SCORE 31

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct llist_node {
    struct llist_node *next;
    struct llist_node *prev;
} llist_node;

typedef struct llist {
    struct llist_node *head;
    struct llist_node *tail;
    int size;
} llist;

enum Suits {
    DENARA,
    COPPE,
    BASTONI,
    SPADE,
};

struct Card_node {
    llist_node node;
    struct Card *c;
};

struct Card {
    int value;
    enum Suits suit;
};

struct PQueue {
    int socket;
    llist queue;
    pthread_mutex_t lock;
    pthread_t pt_id;
};

struct Player_netinfo {
    struct PQueue pk_queue;
    struct sockaddr_in addr;
    socklen_t addr_len;
};

struct Player {
    int id;
    char name[PLAYERNAME_STRLEN + 1];
    int game_score;
    int round_score_thirds; // one point counts for three thirds
    int card_count;
    llist hand;
    struct Player_netinfo netinfo;
};


int fd_unset_nonblocking(int fd, int *flags_ptr);
int fd_set_nonblocking(int fd, int *flags_ptr);
int flush_istream(int fd);

void llist_add(void *list, void *node);
void llist_append(void *list, void *node);
void llist_remove(void *list, void *node);
void llist_init(void *list);
void llist_nuke(void *list, void (fun)(void *node));

char *suit_to_string(enum Suits s);
void initialize_deck(struct Card *cs);
int calculate_pass_value(struct Card **thrown, int n_thrown);
struct Card_node *find_valid_card(struct Card_node *head, int n_cards, int card_id, enum Suits pass_suit);


#endif // COMMON_H