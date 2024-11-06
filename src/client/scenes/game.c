
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include "../../common/platform.h"

#include <raylib.h>

#include "../client_seco.h"

#define SINUSOID_WRONG(amp, freq, phase) \
        (((float)amp) * sin(RAD2DEG * ((float)freq) * ((float)phase)))

#define SINUSOID(amp, freq, phase) \
        (((float)amp) * sin(DEG2RAD * ((float)freq) * ((float)phase)))

extern struct Render_memory *rMem;

void render_status_text(unsigned int len, char *str);
void loading1(Vector2 centre, float scale);
void loading2(Vector2 centre, float scale);
void loading3(Vector2 centre, float scale);

void scene_game(void)
{
    float rotPerSec = 0.03;
    double startAngle = fmod(GetTime() * rotPerSec  * 360, 360);

    const char *str = "scene_game\n";
    DrawText(str, GetScreenWidth()/2 - MeasureText(str, 26)/2, GetScreenHeight()/2, 26, RAYWHITE);
    
}


// void wack() {
//     // RenderTexture2D target = LoadRenderTexture(600, 600);
//     // BeginTextureMode(target);
//     // DrawCrownedCircle((Vector2){300, 300}, 300, 0, RAYWHITE);
//     // EndTextureMode();
//     // DrawTextureRec(target.texture, (Rectangle){0, 0, 600, 600}, (Vector2){0, 0}, WHITE);
    
//     // Image background = GenImageGradientLinear(600, 600, 45, BLUE, DARKPURPLE);
//     // Image ghiera = LoadImageFromTexture(target.texture);
//     // ImageAlphaMask(&background, ghiera);
//     // Texture2D ghieracool = LoadTextureFromImage(background);
//     // UnloadImage(ghiera); UnloadImage(background);
//     // DrawTexture(ghieracool, 100, 0, WHITE);
//     // DrawTexturePro(ghieracool, (Rectangle){-50,0,600,600}, (Rectangle){400,300,600,600}, (Vector2){400,300}, 0,  WHITE);

//     // while (!IsTextureReady(rState.crownTexture))
//         // SetShapesTexture(rState.crownTexture, (Rectangle){0, 0, 99, 128});
//     // Image img = GenImageColor(128, 128, WHITE);
//     // Texture2D tex = LoadTextureFromImage(img);
//     // UnloadImage(img);
//     // SetShapesTexture(tex, (Rectangle){0,0,128,128});
//     // UnloadTexture(tex);
// }


