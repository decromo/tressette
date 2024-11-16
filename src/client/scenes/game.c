
#define _GNU_SOURCE
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <raylib.h>

#include "../client.h"
#include "../../common/common.h"
#include "scene_utils.h"
#include "../client_seco.h"
#include "scenes.h"

void render_status_text(unsigned int len, char *str);
void loading1(Vector2 centre, float scale);
void loading2(Vector2 centre, float scale);
void loading3(Vector2 centre, float scale);

extern struct Render_memory *rMem;

static double startAngle;

static float cardWidthToHeightRatio = 0.605119;
static struct Card cardArr[20];
static Rectangle cardHitbox;
static Vector2 cardOrigins[20];

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
        // debugInfoText((double)endingX);
        // debugInfoText((double)startingX);
    }

    int atlasCardW = rMem->cards.width/10;
    int atlasCardH = rMem->cards.height/4;

    for (int i = 0; i < 40; i++) {
        DrawTexturePro(
            rMem->cards, (Rectangle){i % 10 * atlasCardW, i / 10 * atlasCardH, .width = atlasCardW, .height = atlasCardH},
            (Rectangle){
                .x = cardWidth/2 + startingX + i%10 * (endingX-startingX-cardWidth)/9,
                .y = r.height/8 + i/10 * r.height/4 + r.y,
                .width = cardWidth,
                .height = r.height/4},
            (Vector2){cardWidth/2,r.height/8}, startAngle*2 + (float)GetRandomValue(0,1000)/80.0f - 5.0f, ColorBrightness(WHITE, -0.8));
    }
    // DrawTextureEx(rMem->cards[0], (Vector2){0,0}, 0, 0.2, WHITE);
}
static void fillRecWithSomeCards(Rectangle r, int n_cards, int rows)
{
    // add one to round the full rectangle in card units. we don't add one if the given cards already form a complete rect.
    int cols = n_cards/rows + (n_cards % rows == 0 ? 0 : 1);

    float cardWidth = cardWidthToHeightRatio * r.height/rows;

    // int selectedCardIdx = getSelection(rows, cols, r);

    cardHitbox.width = cardWidth;
    cardHitbox.height = r.height/rows;

    int atlasCardW = rMem->cards.width/10;
    int atlasCardH = rMem->cards.height/4;

    int cardX, cardY;
    for (int i = 0; i < n_cards; i++) {
        cardOrigins[i].x = cardX = r.x + i%cols * (r.width-cardWidth)/(cols-1);
        cardOrigins[i].y = cardY = r.y + i/cols * r.height/rows;

        DrawTexturePro(
            rMem->cards,
            (Rectangle){ atlasCardW*cardArr[i].value, atlasCardH*cardArr[i].suit, atlasCardW, atlasCardH},
            (Rectangle){
                .x = cardWidth/2 + cardX,
                .y = r.height/(2*rows) + cardY,
                cardHitbox.width, cardHitbox.height},
            (Vector2){cardHitbox.width/2,cardHitbox.height/2},
            CheckCollisionPointRec(
                GetMousePosition(),
                (Rectangle){
                    r.x + i%cols * (r.width-cardWidth)/(cols-1), r.y + i/cols * r.height/rows,
                    cardWidth, r.height/rows}
            ) ? (float)GetRandomValue(0,1000)/100.0f - 5 : 0,
            ColorBrightness(WHITE, -0.8));
    }
}

int scene_game_selectCard(void *arg)
{
    struct Game_client *g = arg;

    Vector2 pos = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < g->player.hand.size; i++) {
            if (true == CheckCollisionPointRec(pos,
                    (Rectangle){ cardOrigins[i].x, cardOrigins[i].y, cardHitbox.width, cardHitbox.height }))
                return i;
        }
    }
    return -1;
}

void cardFan(void) {
    int atlasCardW = rMem->cards.width/10;
    int atlasCardH = rMem->cards.height/4;

    Vector2 center = (Vector2){GetScreenWidth()/2, GetScreenHeight()/16 * 15};
    DrawCircleV(center, 10, RED);

    const float factor = 200.0;
    const float maxang = 65.0;
    #define MAXREC 10

    float weights[MAXREC] = {0};
    for (int i = 0; i < MAXREC; i++) {
        weights[i] = 1.0;
    }
    weights[2] = 1.3;
    // weights[6] = 0;

    float totweights = 0;
    for (int i = 0; i < MAXREC; i++) {
        totweights += weights[i] == 1 ? 1 : weights[i]*2;
    }
    float share = MAXREC / (float)totweights;
    debugInfoText("share", (double)share);
    debugInfoText("totweights", (double)totweights);



    float x, y;
    float w = 0;
    for (int i = 0; i < MAXREC; i++) {
        float t = 0;
        if (i != 0 && weights[i-1] > weights[i]) 
            w += weights[i-1];
        else 
            w += weights[i];
        t = w * share/MAXREC;
        debugInfoText(TextFormat("t%f w%f", t, w), (double)i);
        float ang = (t * 2 - 1) * maxang;
        x = center.x + factor*cos(DEG2RAD * (ang + 90));
        y = center.y - factor*sin(DEG2RAD * (ang + 90));
        DrawCircle(x, y, 5, ColorFromHSV(t, 1, 1));
        int xSize = 100 * weights[i];
        int ySize = 180 * weights[i];
        DrawRectanglePro(
            (Rectangle){x, y, xSize, ySize},
            (Vector2){xSize/2,ySize},
            -ang,
            ColorFromHSV(t * 360, 1, 1)
        );
    }
    debugInfoText("x", (double)x);
    debugInfoText("y", (double)y);
}

void scene_game(void* arg)
{
    float rotPerSec = 0.03;
    startAngle = fmod(GetTime() * rotPerSec  * 360, 360);

    struct Game_client *g = arg;
    struct Card_node *cn = (struct Card_node*)g->player.hand.head;
    for (int i = 0; i < g->player.hand.size; i++) {
        assert(cn != NULL);
        cardArr[i] = *cn->c;
        cn = (struct Card_node*)cn->node.next;
    }

    {
        float factor = 0.8;
        float screenW = GetScreenWidth();
        float screenH = GetScreenHeight();
        Rectangle drawRec = (Rectangle){
            screenW*(1-factor)/2,
            screenH*(1-factor)/2,
            screenW * factor,
            screenH * factor};

        int n_cards = g->player.card_count;
        int rows = 4;
        int cols = n_cards/rows + (n_cards % rows == 0 ? 0 : 1);

        // float diffOfRatios = drawRec.width / drawRec.height - cardWidthToHeightRatio*cols/rows;
        // while (diffOfRatios > 0 && rows > 1) {
        //     diffOfRatios -= cardWidthToHeightRatio*cols/(rows-1) - cardWidthToHeightRatio*cols/rows;
            // diffOfRatios += cardWidthToHeightRatio*10 / (-rows-pow(rows, 2));

        float ratioOfRatios = (drawRec.width / drawRec.height) / (cardWidthToHeightRatio*cols/rows);
        float newRor;
        while (ratioOfRatios > 1 && rows > 1) {
            newRor = (drawRec.width / drawRec.height) / (cardWidthToHeightRatio*cols/(rows-1));
            if (newRor < 0.85) break;
            ratioOfRatios = newRor;
            rows--;
            cols = n_cards/rows + (n_cards % rows == 0 ? 0 : 1);
        };
        debugInfoText("ror", (double)ratioOfRatios);

        fillRecWithSomeCards(drawRec, n_cards, rows);
    }

    cardFan();
    
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


