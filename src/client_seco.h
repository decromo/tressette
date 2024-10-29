#ifndef CLIENT_SECO_H
#define CLIENT_SECO_H

#include <raylib.h>
#include <stdio.h>

struct Render_memory {
    int allocSize;
    char statusStr[512];
    RenderTexture2D ferrule;
    RenderTexture2D crown_circs;
    RenderTexture2D asd;

};

extern struct Render_memory *rMem;

static inline void memInit() {
    printf("init!\n");
    rMem->ferrule = LoadRenderTexture(800, 600);
    rMem->crown_circs = LoadRenderTexture(800, 600);
}


#endif // CLIENT_SECO_H