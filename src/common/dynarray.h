#include <stdlib.h>
#include <string.h>
#include <assert.h>

// FIXME: reallocing makes sense only if no more than one function holds the dynarray variable;
// passing it by reference and dereferencing it when calling macros could work but it's not tested.
// Should work fine as long as insertions are done only by the owner when there are no direct references.

struct dynarray {
    size_t n;
    size_t cap;
    size_t el_size;
    char data[];
}/* __attribute__((packed)) */;

#define dynarray(T) T*
#define da_elem_size(da) *(((char*)(da))-(1*sizeof(size_t)))
#define da_capacity(da)  *(((char*)(da))-(2*sizeof(size_t)))
#define da_count(da)     *(((char*)(da))-(3*sizeof(size_t)))

#define da_init(T, init_count) _dynarray_init(sizeof(T), (init_count))
#define da_set(da, idx, elem_ptr) _dynarray_set((void**)&(da), (idx), (elem_ptr))
#define da_set_v(da, idx, value) do { \
        assert(da_elem_size((da)) == sizeof((value))); \
        da_set((da), (idx), &(typeof((value))[1]){(value)}); \
    } while (0)
#define da_append(da, elem_ptr) da_set((da), da_count((da)), (elem_ptr))
#define da_append_v(da, value) do { \
        assert(da_elem_size((da)) == sizeof((value))); \
        da_append((da), &(typeof((value))[1]){(value)}); \
    } while (0)

#define _da_check_realloc(__da, __needed_cap, __where) \
    if ((__da)->cap < (__needed_cap)) { \
        size_t new_capacity = ((__needed_cap)*1.5) + 1; \
        void *res = realloc((__da), sizeof(*(__da)) + sizeof(char[(__da)->el_size][new_capacity])); \
        if (res == NULL) { \
            exit(1); \
        } \
        (__da) = res; \
        (__da)->cap = new_capacity; \
        *(__where) = (__da)->data; \
    }

static inline void *_dynarray_init(size_t element_size, size_t init_count) {
    struct dynarray *da = calloc(1, sizeof(struct dynarray) + sizeof(char[init_count][element_size]));
    *da = (struct dynarray){ .n = 0, .cap = init_count, .el_size = element_size };
    return da->data;
}

static inline void _dynarray_set(void *data_ptr[static 1], size_t index, void *elem) {
    assert(data_ptr != NULL);
    assert(*data_ptr != NULL);
    struct dynarray *da = (*data_ptr - sizeof(*da));

    // Expand the array size if needed with realloc
    _da_check_realloc(da, index, data_ptr);

    // Copy new element in desired index
    char (*data)[da->el_size] = (void*)da->data;
    memcpy(&data[index], elem, da->el_size);
    if (index >= da->n) da->n = index+1;
}

// example usage:
/* 
int main() {
    dynarray(int) da = da_init(int, 2);
    for (int i = 0; i < 10; i++) {
        printf("%d - n=%d - cap=%d\n", i, da_count(da), da_capacity(da));
        da_append_v(da, i);
        assert(i+1 == da_count(da));
    }
    printf("10 - cap=%d - n=%d\n", da_capacity(da), da_count(da));

    for (int i = 0; i < 7; i++) {
        printf("da %d = %d\n", i, da[i]);
    }
}
*/