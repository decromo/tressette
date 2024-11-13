
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <raylib.h>

#include "../client.h"
#include "../../common/common.h"
#include "../client_seco.h"
#include "scenes.h"

void render_status_text(unsigned int len, char *str);
void loading1(Vector2 centre, float scale);
void loading2(Vector2 centre, float scale);
void loading3(Vector2 centre, float scale);

extern struct Render_memory *rMem;
static float cardWidthToHeightRatio = 0.605119;
static double startAngle;


void fillRecWithAllCards(Rectangle r)
{
    float startingX = r.x;
    float endingX = r.x + r.width;
    float cardWidth = cardWidthToHeightRatio * r.height/4;
    bool centerHorizontally = ((float)r.width / (float)r.height > cardWidthToHeightRatio*10/4);
    if (centerHorizontally) {
        int shift = (r.width - cardWidth*10) / 2;
        startingX += shift;
        endingX -= shift;
        debugInfoText((float)endingX);
        debugInfoText((float)startingX);
    }

    for (int i = 0; i < 40; i++) {
        DrawTexturePro(
            rMem->cards[i], (Rectangle){0, 0, .height = rMem->cards[i].height, .width = rMem->cards[i].width},
            (Rectangle){
                .x = cardWidth/2 + startingX + i%10 * (endingX-startingX-cardWidth)/9,
                .y = r.height/8 + i/10 * r.height/4 + r.y,
                .width = cardWidth,
                .height = r.height/4},
            (Vector2){cardWidth/2,r.height/8}, startAngle*2 + (float)GetRandomValue(0,1000)/80.0f - 5.0f, ColorBrightness(WHITE, -0.8));
    }
    // DrawTextureEx(rMem->cards[0], (Vector2){0,0}, 0, 0.2, WHITE);
}
void fillRecWithCards(Rectangle r, int n_cards, struct Card arr[n_cards])
{
    int rows = 4;
    float diffOfRatios = ((float)r.width / (float)r.height - cardWidthToHeightRatio*10/rows);
    while (diffOfRatios > 0) {
        rows--;
        diffOfRatios = ((float)r.width / (float)r.height - cardWidthToHeightRatio*10/rows);
    };
    // add one to round the full rectangle in card units. we don't add one if the given cards already form a complete rect.
    int cols = n_cards/rows + (n_cards % rows == 0 ? 0 : 1);
    float cardWidth = cardWidthToHeightRatio * r.height/rows;

    for (int i = 0; i < n_cards; i++) {
        DrawTexturePro(
            rMem->cards[arr[i].suit*10+arr[i].value], (Rectangle){0, 0, .height = rMem->cards[0].height, .width = rMem->cards[0].width},
            (Rectangle){
                .x = cardWidth/2 + r.x + i%cols * (r.width-cardWidth)/(cols-1),
                .y = r.height/(2*rows) + i/cols * r.height/rows + r.y,
                .width = cardWidth,
                .height = r.height/rows},
            (Vector2){cardWidth/2,r.height/(2*rows)}, startAngle*2 + (float)GetRandomValue(0,1000)/100.0f, ColorBrightness(WHITE, -0.8));
    }
}

void scene_game(void* arg)
{
    float rotPerSec = 0.03;
    startAngle = fmod(GetTime() * rotPerSec  * 360, 360);

    struct Game_client *g = arg;
    struct Card cardArr[20];
    struct Card_node *cn = (struct Card_node*)g->player.hand.head;
    for (int i = 0; i < g->player.hand.size; i++) {
        assert(cn != NULL);
        cardArr[i] = *cn->c;
        cn = (struct Card_node*)cn->node.next;
    }

    float factor = 1.1;
    float screenW = GetScreenWidth();
    float screenH = GetScreenHeight();
    float recW = screenW * factor;
    float recH = screenH * factor;
    fillRecWithCards((Rectangle){screenW*(1-factor)/2, screenH*(1-factor)/2, recW, recH}, g->player.card_count, cardArr);

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


