#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include <raylib.h>

#include "../client_seco.h"

extern struct Render_memory *rMem;

void debugInfoText(double what);

#define SINUSOID_WRONG(amp, freq, phase) \
        (((float)amp) * sin(RAD2DEG * ((float)freq) * ((float)phase)))

#define SINUSOID(amp, freq, phase) \
        (((float)amp) * sin(DEG2RAD * ((float)freq) * ((float)phase)))
        
static inline void render_image(Image *i) {
    rMem->debug_texture = LoadTextureFromImage(*i);
    DrawTexture(rMem->debug_texture, 0, 0, WHITE);
}