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
#include "scene_vtable.h"

void loading1(Vector2 centre, float scale);
void loading2(Vector2 centre, float scale);
void loading3(Vector2 centre, float scale);

extern struct Render_memory *rMem;

static double startAngle;

static int cardCount;
static struct Card cardArr[20];

static Texture2D atlas;
static int atlasCardW;
static int atlasCardH;

static float angs[MAXREC];
static Rectangle recs[MAXREC];

static int hoverCardFan(void) 
{
    int ret = -1;

    Image img = LoadImageFromTexture(rMem->cardFanBuf.texture);
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

static float easing(float *in) 
{
    float out = *in;
    if (out > 1) out = 1;
    if (out < 0) out = 0;
    *in = out;
    
    return out;
}
static void updateCardFan(void)
{
    const int hoveringId = rMem->cardFanHoveringId;
    float *ts = rMem->cardFanTs;

    const float growFactor = 1.6;

    float new_ts, new_seg, prev_seg;
    float ws[MAXREC] = {0};
    float totalWeight = 0;

    for (int i = 0; i < cardCount; i++) {
        // animate depending on selection
        new_ts = 
            (i == hoveringId) 
            ? ts[i] + 0.1      // grow
            : ts[i] - 0.1;     // shrink
        
        new_seg = 1 + (easing(&new_ts)) * (growFactor-1);
        ts[i] = new_ts;

        // add previous segment weighting to total (first element has no previous segment)
        if (i != 0)
            if (prev_seg > new_seg)
                totalWeight += prev_seg;
            else
                totalWeight += new_seg;

        // update array and trailing value
        ws[i] = prev_seg = new_seg;
    }


    debugInfoText(TextFormat("cardcount%d totalw", cardCount), (double)totalWeight);

    // Parameters
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    Vector2 center = (Vector2){sw/2, sh + (sw*1.5)};
    const float distance = sqrt(sh * sh * 0.02) + (sw*1.5);      // center to card origin distance
    const float scale = 200.0;                // card scale
    const float acos_arg = center.x*0.7 * ((float)cardCount / 20) / (scale + distance);
    // const float acos_arg = center.x / ((float)sw/1600) * ((float)cardCount / 20) / (scale + distance);
    const float maxAngMag = PI/2 - acos(acos_arg);    // fan out cards from -maxang to +maxang
    debugInfoText(TextFormat("cx%.2f cy", center.x), (double)(center.y));
    debugInfoText(TextFormat("distance %.2f maxang%.2f acos-arg", distance, maxAngMag), (double)(acos_arg));

    // dbg
    DrawCircleV(center, 10, RED);
    debugInfoText("hover:", (double)hoveringId);

    float x, y, t, ang, height;
    float acc = 0;

    // Compute angles and sizes
    for (int i = 0; i < cardCount; i++) {
        t      = acc / totalWeight;
        ang    = maxAngMag * (-t * 2 + 1);                  // from maxang to -maxang in radians
        x      = center.x + distance*cos(ang + PI/2);
        y      = center.y - distance*sin(ang + PI/2);
        height = scale * ws[i];
        // debugInfoText(TextFormat("i%d acc%.2f ws%.2f x%.2f y%.2f ang%.2f t", i, acc, ws[i], x, y, ang), (double)t);

        // dbg: draw card origins
        DrawCircle(x, y, 5, GREEN);

        // store computations for both draw calls
        angs[i] = RAD2DEG * -ang;           // Raylib's rotation argument for drawing shapes start at 90° and goes cw :/
        recs[i] = (Rectangle){ x, y, height * cardWidthToHeightRatio, height };
        
        // add to accumulator the next segment's weight (it is the biggest value between this and next card's weight) for use in the next card's loop
        if (ws[i+1] > ws[i])
            acc += ws[i+1];
        else
            acc += ws[i];
    }
}
static void drawCardTextures(RenderTexture2D *buf)
{
    if (buf) {
        BeginTextureMode(*buf);
        ClearBackground(BLANK);
    }

    for (int i = 0; i < cardCount; i++)
        DrawTexturePro(
            atlas,
            (Rectangle){ atlasCardW*cardArr[i].value, atlasCardH*cardArr[i].suit, atlasCardW, atlasCardH },
            recs[i],
            (Vector2){recs[i].width / 2, recs[i].height},
            angs[i],
            ColorBrightness(WHITE, -0.3)
        );

    if (buf) EndTextureMode();
}
static void drawCardBoxes(RenderTexture2D *buf) 
{
    if (buf) {
        BeginTextureMode(*buf);
        ClearBackground(BLANK);
    }

    // Draw same regions as colored "hitboxes" to a render buffer
    float xRatio = 1; /* (float)buf->texture.width/GetScreenWidth(); */ 
    float yRatio = 1; /* (float)buf->texture.height/GetScreenHeight(); */
    Color redColor = {0, 0, 0, 255};
    const float blueStep = (float)255/cardCount;  // this is just for display purposes
    for (int i = 0; i < cardCount; i++) {
        // red channel is for the clickable button, blue channel is for humans to see
        redColor.r = 1+i; 
        redColor.b = (1+i)*blueStep; 

        DrawRectanglePro(
            (Rectangle){ recs[i].x * xRatio, recs[i].y * yRatio, recs[i].width * xRatio, recs[i].height * yRatio },
            (Vector2){ recs[i].width * xRatio / 2, recs[i].height * yRatio},
            angs[i],
            redColor
        );
    }

    if (buf) EndTextureMode();
}
static void drawStatusText(void)
{
    if (rMem->statusCD <= 0) return;

    rMem->statusCD--;
    float textLen = MeasureText(rMem->statusStr, 26);
    float textX = (GetScreenWidth()/2) - textLen/2;
    float textY = GetScreenHeight()/3;
    float textSize = 26;
    DrawRectangle(textX-4, textY-4, textLen+8, textSize+8, Fade(GetColor(0x181818ff), 0.8));
    DrawText(rMem->statusStr, textX, textY, textSize, GREEN);
}

int scene_game_selectCard(void *arg) 
{
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

void setStatusText(const char *str)
{
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
    struct Game_client *g = arg;

    float rotPerSec = 0.03;
    startAngle = fmod(GetTime() * rotPerSec  * 360, 360);

    atlas = rMem->cardAtlas;
    atlasCardW = atlas.width/10;
    atlasCardH = atlas.height/4;

    cardCount = g->player.hand.size;

    struct Card_node *cn = (struct Card_node*)g->player.hand.head;
    for (int i = 0; i < cardCount; i++) {
        assert(cn != NULL);
        cardArr[i] = *cn->c;
        cn = (struct Card_node*)cn->node.next;
    }

    updateCardFan();
    drawCardBoxes(&rMem->cardFanBuf);
    drawCardTextures(NULL);
    rMem->cardFanHoveringId = hoverCardFan();
    if (IsKeyDown(KEY_H)) {
        DrawTextureRec(rMem->cardFanBuf.texture, (Rectangle){0, 0, rMem->cardFanBuf.texture.width, -rMem->cardFanBuf.texture.height}, (Vector2){0, 0}, WHITE);
    }
    
    drawStatusText();
}
