#ifndef CLIENT_SECO_H
#define CLIENT_SECO_H

#include <raylib.h>
#include <stdio.h>

#include "scenes/scene_vtable.h"
#include "../common/common.h"

typedef struct FilteredTexture {
    Image bg;
    RenderTexture2D mask;
    Texture2D res;
} FilteredTexture;

struct Render_memory {
    int allocSize;
    int statusCD;
    char statusStr[512];
    RenderTexture2D ferrule;
    FilteredTexture crown;

    enum Scene_tags scene;
    struct Scene_vtable *scene_vtable;

    Texture2D cardAtlas;
    int atlasCardW;
    int atlasCardH;

    RenderTexture2D cardFanBuf;
    char cardFanHoveringId;
    char cardFanClickedId;
    float cardFanTs[20]; // FIXME: pickup MAXREC

    bool have_to_move;

    // Texture2D asd;
    Texture2D debug_texture;
};

void render_loop(void*);
void render_status_text(size_t len, char *str);

void switchScene(enum Scene_tags s);

int scene_game_selectCard(void *arg);


#endif // CLIENT_SECO_H