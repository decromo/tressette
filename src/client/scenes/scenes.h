#ifndef SCENES_H
#define SCENES_H

#include <stdlib.h>
#include <raylib.h>

#include "../../common/common.h"

#define SCENE_VFUNCS \
    F(scene_connection, void, void) \
    F(scene_game, void, void)

#define F(name, ret, ...) ret name(__VA_ARGS__);
SCENE_VFUNCS
#undef F

enum Scene_tags {
    #define F(name, ...) TAG_##name,
    SCENE_VFUNCS
    #undef F
};

struct Scene_vtable {
    size_t capacity;
    size_t size;
    void (*table[])(void);
}__attribute__((packed));

static inline void scene_vtable_gen(struct Scene_vtable **t) {
    if (*t == NULL) {
        size_t starting_capacity = 4;
        *t = (struct Scene_vtable*)RL_MALLOC(sizeof(**t) + sizeof(typeof((**t).table[0])) /* * starting_capacity */);
        (*t)->capacity = starting_capacity;
    }
    void *func_ptr = NULL;
    #define F(name, ...) \
        func_ptr = (void*)name; \
        dynarray_insert((void**)t, sizeof((**t).table[0]), TAG_##name, &func_ptr);
    SCENE_VFUNCS
    #undef F
}



#endif // SCENES_H