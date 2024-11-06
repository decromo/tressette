#ifndef CLIENT_SECO_H
#define CLIENT_SECO_H

#include <raylib.h>
#include <stdio.h>

#include "scenes/scenes.h"

typedef struct FilteredTexture {
    Image bg;
    RenderTexture2D mask;
    Texture2D res;
} FilteredTexture;

struct Render_memory {
    int allocSize;
    char statusStr[512];
    RenderTexture2D ferrule;
    FilteredTexture crown;

    enum Scene_tags scene;
    struct Scene_vtable *scene_vtable;

    Texture asd;
    Texture2D debug_texture;
};

extern struct Render_memory *rMem;
void render_loop(void);
void render_status_text(unsigned int len, char *str);

static inline void memInit() {
    printf("init!\n");
    struct Render_memory *m = rMem;

    m->ferrule = LoadRenderTexture(800, 600);
    {
        m->crown.bg = GenImageColor(800, 600, BLANK);
        m->crown.mask = LoadRenderTexture(m->crown.bg.width, m->crown.bg.height),
        m->crown.res = LoadTextureFromImage(m->crown.bg);
    }

    rMem->scene = TAG_scene_connection;
}


#endif // CLIENT_SECO_H