
#include <string.h>
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#ifdef __MINGW32__
#   include "windef.h"
#endif  // __MINGW32__

#include <raylib.h>

#include "common.h"
#include "client_seco.h"

#define SINUSOID_WRONG(amp, freq, phase) \
        (((float)amp) * sin(RAD2DEG * ((float)freq) * ((float)phase)))

#define SINUSOID(amp, freq, phase) \
        (((float)amp) * sin(DEG2RAD * ((float)freq) * ((float)phase)))

struct Render_memory *rMem = NULL;

void render_init(void **memPtr) {
    assert(memPtr != NULL);

    rMem = *memPtr;
    if (rMem != NULL && rMem->allocSize == sizeof(*rMem)) {
        return;
    }

    void *res = NULL;
    do {
        res = realloc(rMem, sizeof(struct Render_memory));
        if (res != NULL) break;
        TraceLog(LOG_ERROR, "Could not realloc render memory.\n");
        sleep(1);
    } while (true);
    
    *memPtr = rMem = res;
    rMem->allocSize = sizeof(*rMem);

    printf("realloc!\n");
    memInit();

    return;
}
Image GenImageGradientLinearGood(int width, int height, int direction, Color start, Color end)
{
    Color *pixels = (Color *)RL_MALLOC(width*height*sizeof(Color));

    float radianDirection = (float)(90 - direction)/180.f*3.14159f;
    float cosDir = cosf(radianDirection);
    float sinDir = sinf(radianDirection);

    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            // Calculate the relative position of the pixel along the gradient direction
            float pos = (i*cosDir + j*sinDir)/(fabs(width*cosDir) + fabs(height*sinDir));

            float factor = pos;
            factor = (factor > 1.0f)? 1.0f : factor;  // Clamp to [0,1]
            factor = (factor < -1.0f)? -1.0f : factor;  // Clamp to [0,1]

            // Generate the color for this pixel
            pixels[j*width + i].r = (int)((float)end.r*factor + (float)start.r*(1.0f - factor));
            pixels[j*width + i].g = (int)((float)end.g*factor + (float)start.g*(1.0f - factor));
            pixels[j*width + i].b = (int)((float)end.b*factor + (float)start.b*(1.0f - factor));
            pixels[j*width + i].a = (int)((float)end.a*factor + (float)start.a*(1.0f - factor));
        }
    }

    Image image = {
        .data = pixels,
        .width = width,
        .height = height,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };

    return image;
}

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

void DrawCrownedCircle(Vector2 center, float scale, float angle, Color color) {
    const float cro2cirRatios[] = {160.0/ 215.0, 170.0 / 220.0};
    const float croThickRatio = 215.0 / 220.0;
    const float cirThickRatio = 160.0 / 170.0;

    // 0:internal, 1:external
    float crownRadii[] = {croThickRatio, 1};
    float circRadii[] = {cro2cirRatios[0] * crownRadii[0], cro2cirRatios[1] * crownRadii[1]};

    // crown does 68 degrees per segment
    DrawRing(center, circRadii[0] * scale, circRadii[1] * scale, angle, angle + 360, 1, color);
    DrawRing(center, crownRadii[0] * scale, crownRadii[1] * scale, angle, angle + 360.0 * 4, 21, color);
}

void debugInfoText(double startAngle) {
    // DrawRing( (Vector2){.x = -100, .y = 700}, 400, 550, 0, 360, 1, RAYWHITE);
    DrawText(TextFormat("mouse: %d %d", GetMouseX(), GetMouseY()), 0, 0, 26, GREEN);
    DrawText(TextFormat("angle: %0.2f", startAngle), 640, 0, 26, GREEN);
}

Texture2D gradient_bg()
{
    // BeginBlendMode(BLEND_SUBTRACT_COLORS);
    Image bg = GenImageGradientLinear(800, 600, 30, (Color){BLUE.r, BLUE.g, BLUE.b, 64}, (Color){DARKPURPLE.r, DARKPURPLE.g, DARKPURPLE.b, 64});
    Texture2D bg_tx = LoadTextureFromImage(bg);
    UnloadImage(bg);
    DrawTexture(bg_tx, 0, 0, WHITE);
    return bg_tx;
    // EndBlendMode();
}
Texture2D colored_squares()
{
    Image squares = GenImageColor(300, 300, WHITE);
    Texture2D square_tx = LoadTextureFromImage(squares);
    UnloadImage(squares);
    DrawTexture(square_tx, 300, 200, Fade(RED, 0.8));
    DrawTexture(square_tx, 250, 150, Fade(GREEN, 0.8));
    return square_tx;
}
void crowns(double startAngle)
{
    BeginTextureMode(rMem->crown_circs);
    ClearBackground(BLANK); // TODO commenting this out has potential..
    float lsize = 900;
    float rsize = 400;

    Vector2 lc = {150, 900},
            rc = {900, -80};

    DrawCrownedCircle(lc, lsize, startAngle, Fade(SKYBLUE, 0.4));
    DrawCrownedCircle(rc, rsize, startAngle, Fade(SKYBLUE, 0.4));
    EndTextureMode();
    DrawTextureRec(rMem->crown_circs.texture, (Rectangle){0,0,800,-600}, (Vector2){0,0}, WHITE);
}

void rotate_ferrule(RenderTexture2D *ferrule, double angle) {
    BeginTextureMode(*ferrule);
    ClearBackground(BLANK);
    int dim = ferrule->texture.height/2;
    DrawCrownedCircle((Vector2){dim, dim}, dim, angle, WHITE);
    EndTextureMode();
}

void render_loop() {
    SetTraceLogLevel(LOG_WARNING);
    BeginDrawing();
    ClearBackground(GetColor(0x18181800));

    Texture2D shapeWhiteTx = GetShapesTexture();
    Rectangle shapeWhiteTxRec = GetShapesTextureRectangle();

    float rotPerSec = 0.03;
    double startAngle = fmod(GetTime() * rotPerSec  * 360, 360);


    float radiusGreen = SINUSOID_WRONG(3, 1/128, startAngle) + 3 + 6;
    DrawCircleGradient(60, 491, radiusGreen, RAYWHITE, LIME);

    // BeginBlendMode(BLEND_SUBTRACT_COLORS);
    Texture2D bg_tx;
    // bg_tx = gradient_bg();
    // EndBlendMode();

    Texture2D square_tx;
    // square_tx= colored_squares();
    
    void alphablend(double angle);
    alphablend(startAngle);
    // rotate_ferrule(&rMem->ferrule, startAngle);
    // DrawTexturePro(rMem->ferrule.texture, (Rectangle){0,0,600,600}, (Rectangle){400,300,600,600}, (Vector2){300,300}, 0, WHITE);
    DrawTexture(rMem->ferrule.texture, 0, 0, WHITE);

    // SetShapesTexture(bg_tx, (Rectangle){0,0,128,128});
    void loading1(), loading2(), loading3();
    // loading3();
    // SetShapesTexture(shapeWhiteTx, (Rectangle){0,0,128,128});

    crowns(startAngle);

    debugInfoText(startAngle);
    DrawText(rMem->statusStr, 80, 480, 26, RAYWHITE);

    EndDrawing();

    // UnloadTexture(bg_tx);
    // UnloadTexture(square_tx);
    // UnloadRenderTexture(crown_circs);
}

void alphablend(double angle) {
    Image background = GenImageGradientLinearGood(800, 600, angle, BLUE, DARKPURPLE);
    printf("sin:%f cos:%f\n", sin((float)(90-143)/180.f*3.14159f), cos((float)(90-143)/180.f*3.14159f));
    printf("w:%f h:%f\n", cos((float)(90-143)/180.f*3.14159f) * 800, sin((float)(90-143)/180.f*3.14159f)* 600);
    rotate_ferrule(&rMem->ferrule, angle);
    Image ghiera = LoadImageFromTexture(rMem->ferrule.texture);
    // ImageAlphaMask(&background, ghiera);
    
    UpdateTexture(rMem->ferrule.texture, background.data); // TODO: cool effect when reading pixels with different row lenght 
    UnloadImage(ghiera);
    UnloadImage(background);
    // DrawTexture(ghieracool, 100, 0, WHITE);
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

// void loading1() {
//     double loadingAnimTime = fmod(GetTime() * 360, 720.0);
//     double loadingAnimSin = SINUSOID_WRONG(360, 0.00005, GetTime()* 360);
//     DrawRing( 
//         (Vector2){.x = 600, .y = 300},
//         140, 150,
//         loadingAnimTime + 1*(loadingAnimSin),
//         360 + loadingAnimTime + -1*(loadingAnimSin),
//         12, RAYWHITE);
// }

// void loading2() {
//     double loadingAnimTime = fmod(GetTime() * 360, 720.0);
//     double loadingAnimSin = SINUSOID_WRONG(360, 0.00005, GetTime()* 360);
//     DrawRing( 
//         (Vector2){.x = 600, .y = 300},
//         130, 140,
//         loadingAnimSin + 3.2*(loadingAnimSin),
//         loadingAnimSin + 0.2*(loadingAnimSin),
//         12, RAYWHITE);

//     // double testSinPos = SINUSOID(180, 0.1, GetTime() * 360) + 180;
//     // double testSin = SINUSOID(360, 0.1, GetTime() * 360);
//     // DrawCircle(80, testSinPos, 8, YELLOW);
//     // DrawRing( 
//     //     (Vector2){.x = 200, .y = 300},
//     //     130, 140,
//     //     testSin + 6.4*(testSin),
//     //     testSin + 0.2*(testSinPos),
//     //     48, RAYWHITE);
// }
// void loading3() {
//     double loadingAnimTime = GetTime() * 360;
//     double loadingAnimSin = SINUSOID_WRONG(360, 0.001, GetTime() * 8);
//     DrawRing( 
//         (Vector2){.x = 600, .y = 300},
//         120, 130,
//         loadingAnimTime + 3*(loadingAnimSin),
//         loadingAnimTime + -3*(loadingAnimSin),
//         24, RAYWHITE);
// }