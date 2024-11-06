
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <raylib.h>


#include "../client_seco.h"

#define SINUSOID_WRONG(amp, freq, phase) \
        (((float)amp) * sin(RAD2DEG * ((float)freq) * ((float)phase)))

#define SINUSOID(amp, freq, phase) \
        (((float)amp) * sin(DEG2RAD * ((float)freq) * ((float)phase)))

extern struct Render_memory *rMem;

// Image GenImageGradientLinearGood(int width, int height, int direction, Color start, Color end)
// {
//     Color *pixels = (Color *)RL_MALLOC(width*height*sizeof(Color));

//     float radianDirection = (float)(direction*2)/180.f*3.14159f;
//     float cosDir = cosf(radianDirection);
//     float sinDir = sinf(radianDirection);

//     float startVal = 0.5 - (cosDir*width/2) - (sinDir*height/2);
//     float denom = ((signbit(sinDir) != 0) == (signbit(cosDir) != 0))? fabs(startVal) : fabs(startVal+width*cosDir);
//     for (int i = 0; i < width; i++)
//     {
//         for (int j = 0; j < height; j++)
//         {
//             // Calculate the relative position of the pixel along the gradient direction
//             float pos = (startVal + (i*cosDir + j*sinDir)) / denom;

//             float factor = pos;
//             factor = (factor > 1.0f)? 1.0f : factor;  // Clamp to [-1,1]
//             factor = (factor < -1.0f)? -1.0f : factor;  // Clamp to [-1,1]
//             factor = factor / 2 + 0.5f;

//             // Generate the color for this pixel
//             pixels[j*width + i].r = (int)((float)end.r*factor + (float)start.r*(1.0f - factor));
//             pixels[j*width + i].g = (int)((float)end.g*factor + (float)start.g*(1.0f - factor));
//             pixels[j*width + i].b = (int)((float)end.b*factor + (float)start.b*(1.0f - factor));
//             pixels[j*width + i].a = (int)((float)end.a*factor + (float)start.a*(1.0f - factor));
//         }
//     }
//     Image image = {
//         .data = pixels,
//         .width = width,
//         .height = height,
//         .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
//         .mipmaps = 1
//     };

//     return image;
// }

void DrawFerrule(Vector2 center, float scale, float angle, Color color)
{
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

// Texture2D colored_squares()
// {
//     Image squares = GenImageColor(300, 300, WHITE);
//     Texture2D square_tx = LoadTextureFromImage(squares);
//     UnloadImage(squares);
//     DrawTexture(square_tx, 300, 200, Fade(RED, 0.8));
//     DrawTexture(square_tx, 250, 150, Fade(GREEN, 0.8));
//     return square_tx;
// }

void render_image(Image *i) {
    rMem->debug_texture = LoadTextureFromImage(*i);
    DrawTexture(rMem->debug_texture, 0, 0, WHITE);
}

void crowns(double startAngle)
{
    BeginTextureMode(rMem->crown.mask);
    // ClearBackground(BLANK); // TODO commenting this out has potential..
    float lsize = 900;
    float rsize = 400;
    Vector2 lc = {150, 900},
            rc = {900, -80};
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    DrawFerrule(lc, lsize+5, startAngle, Fade(BLANK, 0.2));
    DrawFerrule(rc, rsize+5, startAngle, Fade(BLANK, 1));
    DrawFerrule(lc, lsize, startAngle, Fade(WHITE, 1));
    DrawFerrule(rc, rsize, startAngle, Fade(WHITE, 1));
    EndBlendMode();
    EndTextureMode();
}
// void rotate_single_ferrule(RenderTexture2D *ferrule, double angle)
// {
//     BeginTextureMode(*ferrule);
//     ClearBackground(BLANK);
//     int dim = ferrule->texture.height/2;
//     DrawFerrule((Vector2){dim, dim}, dim, angle, WHITE);
//     DrawFerrule((Vector2){dim, dim}, dim, angle, WHITE);
//     EndTextureMode();
// }

void alphablend(double angle)
{
    Image bg = GenImageGradientLinear(rMem->crown.bg.width, rMem->crown.bg.height, angle, BLUE, DARKPURPLE);
    crowns(angle);
    Image ghiera = LoadImageFromTexture(rMem->crown.mask.texture);
    ImageAlphaMask(&bg, ghiera);

    UpdateTexture(rMem->crown.res, bg.data);
    UnloadImage(rMem->crown.bg);
    UnloadImage(ghiera);
    rMem->crown.bg = bg;

    // DrawTexture(rMem->asd, 0, 0 ,WHITE);
    // UpdateTexture(rMem->crown_circs.texture, background.data); // TODO: cool effect when reading pixels with different row lenght 
    // DrawTexture(ghieracool, 100, 0, WHITE);
}

void scene_connection(void)
{
    float rotPerSec = 0.03;
    double startAngle = fmod(GetTime() * rotPerSec  * 360, 360);

    alphablend(startAngle);
    DrawTextureRec(rMem->crown.res, (Rectangle){0,0,800,-600}, (Vector2){0,0}, WHITE);
    // rotate_ferrule(&rMem->ferrule, startAngle);
    // DrawTexturePro(rMem->ferrule.texture, (Rectangle){0,0,600,600}, (Rectangle){400,300,600,600}, (Vector2){300,300}, 0, WHITE);
    // DrawTextureRec(rMem->crown_circs.texture, (Rectangle){0,0,rMem->crown_circs.texture.width,rMem->crown_circs.texture.height}, (Vector2){0,-200}, WHITE);

    void loading1(Vector2, float), loading2(Vector2, float), loading3(Vector2, float);
    loading2((Vector2){135, 533}, 0.2);
    // loading1((Vector2){200, 300}, 1);
    // loading3((Vector2){600, 300}, 1);

    DrawText(rMem->statusStr, 175, 510, 26, RAYWHITE);

    // float radiusGreen = SINUSOID_WRONG(3, 1/128, startAngle) + 3 + 6;
    // DrawCircleGradient(110, 530, radiusGreen, RAYWHITE, LIME);

    // BeginBlendMode(BLEND_SUBTRACT_COLORS);
    // Texture2D bg_tx;
    // bg_tx = gradient_bg();
    // EndBlendMode();

    // Texture2D square_tx;
    // square_tx= colored_squares();
    // UnloadTexture(bg_tx);
    // UnloadTexture(square_tx);
    // UnloadRenderTexture(crown_circs);
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

void loading1(Vector2 centre, float scale) {
    double loadingAnimTime = fmod(GetTime() * 360, 720.0);
    double loadingAnimSin = SINUSOID_WRONG(360, 0.00005, GetTime()* 360);
    DrawRing( 
        centre,
        140*scale, 150*scale,
        loadingAnimTime + 1*(loadingAnimSin),
        360 + loadingAnimTime + -1*(loadingAnimSin),
        12, RAYWHITE);
}

void loading2(Vector2 centre, float scale) {
    double loadingAnimTime = fmod(GetTime() * 360, 720.0);
    double loadingAnimSin = SINUSOID_WRONG(360, 0.00005, GetTime()* 360);
    DrawRing( 
        centre,
        125*scale, 140*scale,
        loadingAnimSin + 3.2*(loadingAnimSin),
        loadingAnimSin + 0.2*(loadingAnimSin),
        12, RAYWHITE);

    // double testSinPos = SINUSOID(180, 0.1, GetTime() * 360) + 180;
    // double testSin = SINUSOID(360, 0.1, GetTime() * 360);
    // DrawCircle(80, testSinPos, 8, YELLOW);
    // DrawRing( 
    //     centre,
    //     130*scale, 140*scale,
    //     testSin + 6.4*(testSin),
    //     testSin + 0.2*(testSinPos),
    //     48, RAYWHITE);
}
void loading3(Vector2 centre, float scale) {
    double loadingAnimTime = GetTime() * 360;
    double loadingAnimSin = SINUSOID_WRONG(360, 0.001, GetTime() * 8);
    DrawRing( 
        centre,
        125*scale, 130*scale,
        loadingAnimTime + 3*(loadingAnimSin),
        loadingAnimTime + -3*(loadingAnimSin),
        24, RAYWHITE);
}