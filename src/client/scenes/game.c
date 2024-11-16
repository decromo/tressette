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

void loading1(Vector2 centre, float scale);
void loading2(Vector2 centre, float scale);
void loading3(Vector2 centre, float scale);

extern struct Render_memory *rMem;

static double startAngle;

static struct Card cardArr[20];

static Texture2D cards;
static int atlasCardW;
static int atlasCardH;

static int hoverCardFan(void) {
    int ret = -1;

    Image img = LoadImageFromTexture(rMem->cardFanBuffer.texture);
    // float xRatio = (float)img.width/GetScreenWidth();
    // float yRatio = (float)img.height/GetScreenHeight();
    // debugInfoText("xRatio", (double)xRatio);
    // debugInfoText("yRatio", (double)yRatio);

    Vector2 imgPos;
    Vector2 mousePos = GetMousePosition();
    
    // imgPos.x = mousePos.x * xRatio;
    // imgPos.y = img.height - mousePos.y * yRatio;
    imgPos.x = (mousePos.x > img.width) ? img.width : mousePos.x;
    imgPos.y = (mousePos.y > img.height) ? 0 : img.height - mousePos.y;

    // ret = (GetImageColor(img, imgPos.x, imgPos.y)).r - 1;
    ret = (GetImageColor(img, imgPos.x, imgPos.y)).r - 1;
    UnloadImage(img);

    return ret;
}

static void cardFan(void *arg)
{
    int handSize = ((struct Game_client*)arg)->player.hand.size;

    RenderTexture2D tex = rMem->cardFanBuffer;
    float *weights = rMem->cardFanWeights;
    int id = rMem->cardFanHoveringId;

    float totweights = 0;
    float new_w;
    for (int i = 0; i < handSize; i++) {
        new_w = (i == id) 
            ? weights[i] + 0.1
            : weights[i] - 0.1;
        if (new_w > 1.5) new_w = 1.5;
        if (new_w <= 1) {
            new_w = 1;
            totweights += 1;
        } else {
            totweights += 1 + (new_w-1);
        }
        weights[i] = new_w;
    }
    float share = 1 / (float)(totweights-1);


    Vector2 center = (Vector2){GetScreenWidth()/2, GetScreenHeight()/16 * 15};
    DrawCircleV(center, 10, RED);

    const float factor = 200.0;
    const float maxang = 65.0;

    float x, y;
    float Ew = 0;

    float angs[MAXREC];
    Rectangle recs[MAXREC];
    Vector2 vecs[MAXREC];

    debugInfoText("handsiz", (double)handSize);
    debugInfoText("totweights", (double)totweights);
    // Compute positions and draw textures to screen
    for (int i = 0; i < handSize; i++) {
        float t = 0;
        if (handSize == 1) t = 0.5;
        else {
            t = Ew * share;
        }
        // if (i != handSize-1 && weights[i+1] > weights[i]) 
        //     Ew += weights[i+1];
        // else 
            Ew += weights[i];
        debugInfoText(TextFormat("i%d w%.2f Ew%.2f t", i, weights[i], Ew), (double)t);

        float ang = angs[i] = (t * 2 - 1) * maxang;
        recs[i] = (Rectangle){
            center.x + factor*cos(DEG2RAD * (ang + 90)),
            center.y - factor*sin(DEG2RAD * (ang + 90)),
            100 * weights[i],
            180 * weights[i]
        };
        vecs[i] = (Vector2){recs[i].width / 2, recs[i].height};
        DrawTexturePro(
            cards,
            (Rectangle){ atlasCardW*cardArr[i].value, atlasCardH*cardArr[i].suit, atlasCardW, atlasCardH},
            recs[i],
            vecs[i],
            -angs[i],
            ColorBrightness(WHITE, -0.3)
        );
        DrawCircle(recs[i].x, recs[i].y, 5, GREEN);
    }

    // Draw same regions as colored "hitboxes" to a render buffer
    BeginTextureMode(tex);
    ClearBackground(BLANK);
    for (int i = 0; i < handSize; i++) {
        Color redness = {1+i, 0, i*20, 255}; // blue channel is for humans to see
        DrawRectanglePro(
            recs[i],
            vecs[i],
            -angs[i],
            redness
        );
    }
    EndTextureMode();
}

int scene_game_selectCard(void *arg) {
    (void)arg;
    int ret = -1;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (rMem->cardFanClickedId == rMem->cardFanHoveringId) {
            ret = rMem->cardFanHoveringId;
        } else {
            ret = -1;
        }
        rMem->cardFanClickedId = -1;
    }
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        rMem->cardFanClickedId = rMem->cardFanHoveringId;
        ret = -1;
    }
    return ret;
}

void setStatusText(const char *str) {
    rMem->statusCD = 90;
    
    int len = strlen(str);

    if (len >= 512) len = 512-1;
    strncpy(rMem->statusStr, str, len);
    rMem->statusStr[len] = '\0';

    double maxlen = (GetScreenWidth()*0.8) / 26;
    debugInfoText("maxlen", maxlen);

    linefeed_string(len, rMem->statusStr, 512, maxlen, 2, 4);
}

void scene_game(void* arg)
{
    float rotPerSec = 0.03;
    startAngle = fmod(GetTime() * rotPerSec  * 360, 360);
    cards = rMem->cards;
    atlasCardW = cards.width/10;
    atlasCardH = cards.height/4;

    struct Game_client *g = arg;
    struct Card_node *cn = (struct Card_node*)g->player.hand.head;
    for (int i = 0; i < g->player.hand.size; i++) {
        assert(cn != NULL);
        cardArr[i] = *cn->c;
        cn = (struct Card_node*)cn->node.next;
    }

    cardFan(arg);
    rMem->cardFanHoveringId = hoverCardFan();
    
    // const char *str = "scene_game\n";
    if (rMem->statusCD > 0) {
        rMem->statusCD--;
        float textLen = MeasureText(rMem->statusStr, 26);
        float textX = (GetScreenWidth()/2) - textLen/2;
        float textY = GetScreenHeight()/3;
        float textSize = 26;
        DrawRectangle(textX-4, textY-4, textLen+8, textSize+8, Fade(GetColor(0x181818ff), 0.8));
        DrawText(rMem->statusStr, textX, textY, textSize, GREEN);
    }
}


void fillRecWithAllCards(double startAngle, Texture2D cardTexture, Rectangle r)
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

    int atlasCardW = cardTexture.width/10;
    int atlasCardH = cardTexture.height/4;

    for (int i = 0; i < 40; i++) {
        DrawTexturePro(
            cardTexture, (Rectangle){i % 10 * atlasCardW, i / 10 * atlasCardH, .width = atlasCardW, .height = atlasCardH},
            (Rectangle){
                .x = cardWidth/2 + startingX + i%10 * (endingX-startingX-cardWidth)/9,
                .y = r.height/8 + i/10 * r.height/4 + r.y,
                .width = cardWidth,
                .height = r.height/4},
            (Vector2){cardWidth/2,r.height/8}, startAngle*2 + (float)GetRandomValue(0,1000)/80.0f - 5.0f, ColorBrightness(WHITE, -0.8));
    }
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


