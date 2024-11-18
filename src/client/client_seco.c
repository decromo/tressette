#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include <assert.h>
#include <string.h>
#include <math.h>

#include <raylib.h>

#include "../common/common.h"
#include "client_seco.h"
#include "scenes/scene_vtable.h"

struct Render_memory *rMem = NULL;

static void mem_init() {
    printf("init!\n");
    struct Render_memory *m = rMem;

    m->ferrule = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    m->crown.bg = GenImageColor(GetScreenWidth(), GetScreenHeight(), BLANK);
    m->crown.mask = LoadRenderTexture(m->crown.bg.width, m->crown.bg.height),
    m->crown.res = LoadTextureFromImage(m->crown.bg);

    m->scene = TAG_scene_connection;

    m->cardAtlas = LoadTexture("./assets/half.png");
    m->atlasCardW = m->cardAtlas.width/10;
    m->atlasCardH = m->cardAtlas.height/4;

    m->cardFanBuf = LoadRenderTexture(1600, 900);
}

void render_init(void **memPtr)
{
    assert(memPtr != NULL);

    rMem = *memPtr;
    bool is_first_init = (rMem == NULL);

    if (is_first_init || rMem->allocSize != sizeof(*rMem)) {
        void *res = NULL;
        do {
            res = realloc(rMem, sizeof(*rMem));
            if (res != NULL) break;
            TraceLog(LOG_ERROR, "Could not realloc render memory.\n");
            sleep(1);
        } while (true);
        
        *memPtr = rMem = res;
        rMem->allocSize = sizeof(*rMem);

        printf("realloc!\n");
        mem_init();
    }

    if (is_first_init) rMem->scene_vtable = NULL;
    scene_vtable_gen(&rMem->scene_vtable);

    return;
}

void debugInfoText(const char *what, double value)
{
    static float start = 0;
    // DrawRing( (Vector2){.x = -100, .y = 700}, 400, 550, 0, 360, 1, RAYWHITE);
    DrawText(TextFormat("mouse: %d %d", GetMouseX(), GetMouseY()), 0, 0, 26, GREEN);
    DrawText(TextFormat("%s: %0.2f", what, value), GetScreenWidth() * 0.6, start, 26, GREEN);
    start += 28;
}
void debugValueAdd(const char *name, float *val) {
    // TODO
}

void render_loop(void *arg)
{
    SetTraceLogLevel(LOG_ERROR);
    BeginDrawing();
    ClearBackground(GetColor(0x18181800));

    Texture2D shapeDefaultTx = GetShapesTexture();
    Rectangle shapeDefaultTxRec = GetShapesTextureRectangle();

    float rotPerSec = 0.03;
    double startAngle = fmod(GetTime() * rotPerSec  * 360, 360);

    rMem->scene_vtable->table[rMem->scene](arg);

    if (IsKeyPressed(KEY_S)) {
        rMem->scene = (rMem->scene + 1) % 2;
    }

    debugInfoText("angle", startAngle);

    EndDrawing();

    // if (IsTextureReady(rMem->debug_texture)) UnloadTexture(rMem->debug_texture);
}
void render_status_text(size_t len, char *str) 
{
    const int lineMaxLen = 32;
    const int maxCharsOverMaxLen = 5;
    const int maxPrecNewlineChars = 15;

    if (len >= 512) len = 512-1;
    strncpy(rMem->statusStr, str, len);
    rMem->statusStr[len] = '\0';
    
    linefeed_string(len, rMem->statusStr, 512, lineMaxLen, maxCharsOverMaxLen, maxPrecNewlineChars);
}
void switchScene(enum Scene_tags s) {
    rMem->scene = s;
}

