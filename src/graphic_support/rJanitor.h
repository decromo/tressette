#include <raylib.h>
#include <stdlib.h>

#define RJANITOR_DEFAULT_ARRAY_SIZE 16

typedef struct {
    void (*task) (void);
} rJanitor_elem;

typedef struct {
    int size;

} rJanitor_arr;


#ifdef RJANITOR_IMPLEMENTATION

#endif // RJANITOR_IMPLEMENTATION