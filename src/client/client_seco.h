#ifndef CLIENT_SECO_H
#define CLIENT_SECO_H

#include <raylib.h>
#include <stdio.h>

#include "scenes/scenes.h"
#include "../common/common.h"

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

    Texture2D cards;

    bool have_to_move;

    // Texture asd;
    Texture2D debug_texture;
};

extern struct Render_memory *rMem;
void render_loop(void*);
void render_status_text(unsigned int len, char *str);

static inline void memInit() {
    printf("init!\n");
    struct Render_memory *m = rMem;

    m->ferrule = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    {
        m->crown.bg = GenImageColor(GetScreenWidth(), GetScreenHeight(), BLANK);
        m->crown.mask = LoadRenderTexture(m->crown.bg.width, m->crown.bg.height),
        m->crown.res = LoadTextureFromImage(m->crown.bg);
    }

    m->scene = TAG_scene_connection;

    m->cards = LoadTexture("./assets/half.png");
}

static inline void switchScene(enum Scene_tags s) {
    rMem->scene = s;
}

int scene_game_selectCard(void *arg);


#endif // CLIENT_SECO_H