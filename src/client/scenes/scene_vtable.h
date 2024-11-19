#ifndef SCENES_H
#define SCENES_H

#include <stdlib.h>
#include <raylib.h>

#include "../../common/common.h"

#define SCENE_VFUNCS \
    F(scene_connection, void, void*) \
    F(scene_game, void, void*)
        
#define F(name, ret, ...) ret name(__VA_ARGS__);
SCENE_VFUNCS
#undef F

enum Scene_tags {
    #define F(name, ...) TAG_##name,
    SCENE_VFUNCS
    #undef F
};

typedef void (*Scene_func)(void*);

static inline void scene_vtable_gen(struct Scene_func **t) {
    if (*t == NULL) {
        *t = _dynarray_init(sizeof(Scene_func), (4));
    }
    void *func_ptr = NULL;
    #define F(name, ...) da_set_v(*t, TAG_##name, (void*)name);
    SCENE_VFUNCS
    #undef F
}

#endif // SCENES_H