
#include "scenes/scenes.h"
#include <string.h>
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <raylib.h>

#include "client_seco.h"
#include "scenes/scenes.h"

struct Render_memory *rMem = NULL;

void render_status_text(unsigned int len, char *str) 
{
    const int lineMaxLen = 32;
    const int maxCharsOverMaxLen = 5;
    const int maxPrecNewlineChars = 15;

    if (len >= 512) len = 512-1;
    strncpy(rMem->statusStr, str, len);
    rMem->statusStr[len] = '\0';
    
    linefeed_string(len, rMem->statusStr, 512, lineMaxLen, maxCharsOverMaxLen, maxPrecNewlineChars);
}

void render_init(void **memPtr) {
    assert(memPtr != NULL);

    rMem = *memPtr;
    bool is_first_init = rMem == NULL;

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
        memInit();
    }

    if (is_first_init) rMem->scene_vtable = NULL;
    scene_vtable_gen(&rMem->scene_vtable);

    return;
}

void debugInfoText(double startAngle) {
    // DrawRing( (Vector2){.x = -100, .y = 700}, 400, 550, 0, 360, 1, RAYWHITE);
    DrawText(TextFormat("mouse: %d %d", GetMouseX(), GetMouseY()), 0, 0, 26, GREEN);
    DrawText(TextFormat("angle: %0.2f", startAngle), 640, 0, 26, GREEN);
}

void render_loop() {
    SetTraceLogLevel(LOG_ERROR);
    BeginDrawing();
    ClearBackground(GetColor(0x18181800));

    Texture2D shapeDefaultTx = GetShapesTexture();
    Rectangle shapeDefaultTxRec = GetShapesTextureRectangle();

    float rotPerSec = 0.03;
    double startAngle = fmod(GetTime() * rotPerSec  * 360, 360);

    rMem->scene_vtable->table[rMem->scene]();

    if (IsKeyPressed(KEY_S)) {
        rMem->scene = (rMem->scene + 1) % 2;
    }

    debugInfoText(startAngle);

    EndDrawing();

    // if (IsTextureReady(rMem->debug_texture)) UnloadTexture(rMem->debug_texture);
}