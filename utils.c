#include <stdlib.h>
#include <assert.h>
#include "common.h"

void llist_add(void *list, void *node) {
    llist *ll = list;
    llist_node *n = node;
    assert(ll != NULL && n != NULL);

    if (ll->head == NULL) ll->head = (llist_node*)ll;
    if (ll->tail == NULL) ll->tail = (llist_node*)ll;

    n->next = ll->head;
    ll->head->prev = n;
    n->prev = (llist_node*) ll;
    ll->head = n;

    ll->size++;
}
void llist_append(void *list, void *node) {
    llist *ll = list;
    llist_node *n = node;
    assert(ll != NULL && n != NULL);

    if (ll->head == NULL) ll->head = (llist_node*)ll;
    if (ll->tail == NULL) ll->tail = (llist_node*)ll;

    n->next = (llist_node *) ll;
    n->prev = ll->tail;
    ll->tail->next = n;
    ll->tail = n;

    ll->size++;
}
// this frees the node pointer! (if it's not the list node)
void llist_remove(void *list, void *node) {
    llist *ll = list;
    llist_node *n = node;
    assert(ll != NULL && n != NULL);

    assert(ll->size >= 0);
    if (ll->size == 0 || ll == node) return;
    
    if (n->next != NULL) {
        n->next->prev = n->prev;
    }
    if (n->prev != NULL) {
        n->prev->next = n->next;
    }
    free(n);
    ll->size--;
}

void llist_nuke(void *list, void (fun)(void *node)) {
    llist *ll = list;
    llist_node *n;

    for (int i = 0; i < ll->size; i++) {
        n = ll->head;
        assert(n != NULL);
        if (fun != NULL) {
            fun(n);
        }
        llist_remove(ll, n);
    }
    // just to be sure
    ll->head = (llist_node*)ll;
    ll->tail = (llist_node*)ll;
    ll->size = 0;
}