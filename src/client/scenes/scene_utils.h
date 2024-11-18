#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include <raylib.h>

#include "../client_seco.h"

extern struct Render_memory *rMem;


#define SINUSOID_WRONG(amp, freq, phase) \
        (((float)amp) * sin(RAD2DEG * ((float)freq) * ((float)phase)))
    
#define SINUSOID(amp, freq, phase) \
        (((float)amp) * sin(DEG2RAD * ((float)freq) * ((float)phase)))
        
void debugInfoText(const char *what, double val);
void setStatusText(const char *str);
static inline void render_image(Image *i) {
    rMem->debug_texture = LoadTextureFromImage(*i);
    DrawTexture(rMem->debug_texture, 0, 0, WHITE);
}

// SCENE_GAME SECTION
#define MAXREC 20

static const float cardWidthToHeightRatio = 0.605119;
void fillRecWithAllCards(double startAngle, Texture2D cardTexture, Rectangle r);

