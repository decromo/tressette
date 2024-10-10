#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <fcntl.h>

#include "common.h"

int fd_unset_nonblocking(int fd, int *flags_ptr) {
    int newflags = 0;
    if (flags_ptr == NULL) {
        flags_ptr = &newflags;
    }

    if ((*flags_ptr & O_NONBLOCK) == 0) {
        if (fcntl(fd, F_SETFL, (*flags_ptr & ~O_NONBLOCK)) == -1) {
            perror("fcntl unset nonblock");
            return -1;
        }
    }
    return 0;
}

int fd_set_nonblocking(int fd, int *flags_ptr) {
    int newflags = 0;
    if (flags_ptr == NULL) {
        flags_ptr = &newflags;
    }

    *flags_ptr = fcntl(fd, F_GETFL);
    if (*flags_ptr == -1) {
        perror("fcntl get flags");
        return -1;
    }

    if ((*flags_ptr & O_NONBLOCK) == 0) {
        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl set nonblock");
            return -1;
        }
    }
    return 0;
}

int flush_instream(int fd) {
    int res = 0;
    int flags = 0;
    char c = 'a';
    size_t bytes_read = 0;

    if (fd_set_nonblocking(fd, &flags) == -1) {
        return -1;
    }

    while (bytes_read != -1)
        bytes_read = read(fd, &c, 1);

    if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
        perror("read() failed");
        res = -1;
    }

    if (fd_unset_nonblocking(fd, &flags) == -1) {
        return -1;
    }
    return res;
}

char *suit_to_string(enum Suits s) {
    static char string[64] = "null";
    switch (s) {
    case DENARA:
        strcpy(string, "Denara");
        break;
    case COPPE:
        strcpy(string, "Coppe");
        break;
    case BASTONI:
        strcpy(string, "Bastoni");
        break;
    case SPADE:
        strcpy(string, "Spade");
        break;
    };
    return string;
}
void initialize_deck(struct Card *cs) {
    memset(cs, 0, 40 * sizeof(struct Card));
    for (int i = 0; i < 40; i++) {
        cs[i] = (struct Card){ .value = i % 10, .suit = i / 10 };
    }
}
int calculate_pass_value(struct Card **thrown, int n_thrown) {
    int pass_value = 0;
    for (int i = 0; i < n_thrown; i++) {
        switch (thrown[i]->value) {
        case 1: case 2: case 7: case 8: case 9:
            pass_value += 1;
            break;
        case 0:
            pass_value += 3;
            break;
        }
    }
    return pass_value;
}
struct Card_node *find_valid_card(struct Card_node *head, int n_cards, int card_id, enum Suits pass_suit) {
    if (card_id < 0) {
        fprintf(stderr, "ERRO: Invalid card id recived.\n");
        return NULL;
    }

    bool flying[4] = { true, true, true, true };
    struct Card_node *cn = head;
    for (int i = 0; i < n_cards; i++) {
        assert(cn != NULL);
        flying[cn->c->suit] = false;
        cn = (struct Card_node *)cn->node.next;
    }

    cn = (struct Card_node *)head;
    for (int i = 0; i < n_cards; i++) {
        assert(cn != NULL);
        // printf("flying on %s %s\n", suit_to_string(cn->c->suit), flying[cn->c->suit] ? "true" : "false");
        if (i != card_id) {
            cn = (struct Card_node *)cn->node.next;
            continue;
        }
        if ((pass_suit == -1) || (cn->c->suit == pass_suit) || (flying[pass_suit] == true)) {
            return cn;
        }
        printf("Can't play %d of %s:%d on ", cn->c->value + 1, suit_to_string(cn->c->suit), cn->c->suit);
        printf("%s:%d\n", suit_to_string(pass_suit), pass_suit);

        // TODO: take this out of the shared function
        // char answ = 'a';
        // printf("Would you like to play it anyways? [y/n]");
        // while (true) {
        //     fflush(stdout);
        //     answ = fgetc(stdin);
        //     switch (answ) {
        //         case 'Y': case 'y': 
        //         printf("Don't get caught!"); return cn;
        //         case 'N': case 'n': 
        //         return NULL;
        //         default: printf("Invalid answer, retry: "); 
        //         break;
        //     }
        // }
        // fprintf(stderr, "ERRO: Unplayable card id recived.\n");
        return NULL;
    }
    fprintf(stderr, "ERRO: Could not find card with given id.\n");
    return NULL;
}

// void free_card_node(void *card_node) {
//     struct Card_node *cn = card_node;

// }

// struct Card *find_card_in_deck(struct Card (*deck)[40], int v, enum Suits s) {
    
// }

/*
    TODO:
    -- general
        - implement the bluffing feature
        - make first player the admin
        - have the server acknowledge the clients on connection
        - implement team games

    -- clientside
        - redo printed interface (just a bit)
        - close game when server dies
        - look into cleaning up the recv thread

    -- serverside
        - look into cleaning up the recv thread
*/