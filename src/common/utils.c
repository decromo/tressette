#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "raylib.h"

void llist_add(void *list, void *node) {
    llist *ll = list;
    llist_node *n = node;
    assert(ll != NULL && n != NULL);

    if (ll->head == NULL) ll->head = (llist_node *)ll;
    if (ll->tail == NULL) ll->tail = (llist_node *)ll;

    n->next = ll->head;
    ll->head->prev = n;
    n->prev = (llist_node *)ll;
    ll->head = n;

    ll->size++;
}
void llist_append(void *list, void *node) {
    llist *ll = list;
    llist_node *n = node;
    assert(ll != NULL && n != NULL);

    if (ll->head == NULL) ll->head = (llist_node *)ll;
    if (ll->tail == NULL) ll->tail = (llist_node *)ll;

    n->next = (llist_node *)ll;
    n->prev = ll->tail;
    ll->tail->next = n;
    ll->tail = n;

    ll->size++;
}
// this DOES NOT free the node pointer! (if it's not the list node)
void llist_pop(void *list) {
    assert(list != NULL);
    llist *ll = list;
    llist_node *n = ll->head;
    assert(n != NULL);

    assert(ll->size >= 0);
    if (ll->size == 0 || list == n) return;

    if (n->next != NULL) {
        n->next->prev = n->prev;
    }
    if (n->prev != NULL) {
        n->prev->next = n->next;
    }
    
    free(n);
    ll->size--;

}
// this frees the node pointer! (if it's not the list node)
void llist_remove(void *list, void *node) {
    assert(list != NULL && node != NULL);

    llist *ll = list;
    llist_node *n = node;

    assert(ll->size >= 0);
    if (ll->size == 0 || list == n) return;

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
    ll->head = (llist_node *)ll;
    ll->tail = (llist_node *)ll;
    ll->size = 0;
}

// make sure the list is empty, otherwise the data of its nodes will leak
void llist_init(void *list) {
    assert(list != NULL);

    llist_nuke(list, NULL);
    llist *ll = list;

    ll->head = (llist_node *)ll;
    ll->tail = (llist_node *)ll;
    ll->size = 0;
}

void dynarray_append(void **arr_raw, size_t elem_size, void *elem) {
    struct dynarray {
        size_t capacity;
        size_t size;
        char data[];
    }__attribute__((packed));
    struct dynarray *arr = *arr_raw;

    // Expand the array size if needed with realloc
    if (arr->capacity == arr->size) {
        size_t new_capacity = arr->capacity*1.5 + 1;
        void *res = realloc(*arr_raw, sizeof(*arr) + elem_size*new_capacity);
        if (res == NULL) {
            TraceLog(LOG_FATAL, "Could not realloc dynamic array");
            exit(1);
        }
        *arr_raw = arr = res;
        arr->capacity = new_capacity;
    }

    // Copy new element over
    memcpy(&arr->data[arr->size*elem_size], elem, elem_size);
}
void dynarray_insert(void **arr_raw, size_t elem_size, size_t index, void *elem) {
    struct dynarray {
        size_t capacity;
        size_t size;
        char data[];
    }__attribute__((packed));

    assert(arr_raw != NULL);
    assert(*arr_raw != NULL);
    struct dynarray *arr = *arr_raw;

    // Expand the array size if needed with realloc
    if (index >= arr->capacity) {
        size_t new_capacity = index + 1;
        void *res = realloc(*arr_raw, sizeof(*arr) + elem_size*new_capacity);
        if (res == NULL) {
            TraceLog(LOG_FATAL, "Could not realloc dynamic array");
            exit(1);
        }
        *arr_raw = arr = res;
        arr->capacity = new_capacity;
    }

    // Copy new element in desired index
    memcpy(&arr->data[elem_size*index], elem, elem_size);
}

// inserts newline ('\n') characters, without splitting words if possible.
// Parameters:
//      len, the number of characters in str
//      str, pointer to the string to format
//      bufsiz, the actual size in bytes of the buffer containing said string
//      line_maxlen, the desired optimal character lenght of a single line
//      over_len_maxchars, the acceptable number of characters a line can go over the desired lenght
//      prec_lf_maxchars, the acceptable amount of whitespace remaining in a line when a precocious linefeed is
//          made on said line (i.e. when the newline character is inserted before reaching line_maxlen characters)
void linefeed_string(int len, char *str, size_t bufsiz, int line_maxlen, int over_len_maxchars, int prec_lf_maxchars) {
    // minus one to reserve space for the null terminator
    size_t bufferBytesLeft = bufsiz-1 - len;

    // execute subroutine until the remaining text is shorter than the space available
    while (len > line_maxlen + over_len_maxchars) {
        int spaceBeforeIdx, spaceAfterIdx;
        char *spaceBefore, *spaceAfter;
        spaceBeforeIdx = spaceAfterIdx = 0;
        spaceBefore = spaceAfter = str;

        char *foundSpace;
        while (spaceAfterIdx < line_maxlen) {
            if (NULL == (foundSpace = strchr(spaceAfter+1, ' '))) { break; }
            spaceBefore = spaceAfter;
            spaceBeforeIdx = spaceAfterIdx;
            spaceAfter = foundSpace;
            spaceAfterIdx = labs(str - spaceAfter) /* / sizeof(*renderMemory->statusStr) */;
        }

        char *breakPtr = NULL;
        int breakIdx = -1;

        if (spaceAfter == str || (line_maxlen - spaceBeforeIdx > prec_lf_maxchars)) {
            // this array addressing should not got out of bounds as per the while condition
            memmove(&str[line_maxlen + 1], &str[line_maxlen], len - line_maxlen - (bufferBytesLeft > 0));
            breakPtr = &str[line_maxlen];
            breakIdx = 30;
            bufferBytesLeft--;
            str[len] = '\0'; // just in case
        }
        else if (spaceAfterIdx > line_maxlen + over_len_maxchars) {
            *spaceBefore = '\n';
            breakPtr = spaceBefore;
            breakIdx = spaceBeforeIdx;
        }
        else {
            *spaceAfter = '\n';
            breakPtr = spaceAfter;
            breakIdx = spaceAfterIdx;
        }

        assert(breakPtr != NULL && breakIdx != -1);
        *breakPtr = '\n';
        str = breakPtr+1;
        len -= breakIdx+1;
    }
}