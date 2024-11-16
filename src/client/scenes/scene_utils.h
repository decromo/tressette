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
        
static inline void render_image(Image *i) {
    rMem->debug_texture = LoadTextureFromImage(*i);
    DrawTexture(rMem->debug_texture, 0, 0, WHITE);
}



// SCENE_GAME SECTION
#define MAXREC 20

// this function used to be in a .c file, and relies on some static buffers.
// inline void fillRecWithSomeCards(Texture2D cardTexture, Rectangle r, int n_cards, int rows)
// {
//     // add one to round the full rectangle in card units. we don't add one if the given cards already form a complete rect.
//     int cols = n_cards/rows + (n_cards % rows == 0 ? 0 : 1);

//     // cardWidthToHeightRatio = 0.605119
//     float cardWidth = cardWidthToHeightRatio * r.height/rows;

//     // int selectedCardIdx = getSelection(rows, cols, r);

//     static Rectangle cardHitbox = {0};
//     cardHitbox.width = cardWidth;
//     cardHitbox.height = r.height/rows;

//     static Vector2 cardOrigins[20];
//     static struct Card cardArr[20];

//     int atlasCardW = cardTexture.width/10;
//     int atlasCardH = cardTexture.height/4;

//     int cardX, cardY;
//     for (int i = 0; i < n_cards; i++) {
//         cardOrigins[i].x = cardX = r.x + i%cols * (r.width-cardWidth)/(cols-1);
//         cardOrigins[i].y = cardY = r.y + i/cols * r.height/rows;

//         DrawTexturePro(
//             cardTexture,
//             (Rectangle){ atlasCardW*cardArr[i].value, atlasCardH*cardArr[i].suit, atlasCardW, atlasCardH},
//             (Rectangle){
//                 .x = cardWidth/2 + cardX,
//                 .y = r.height/(2*rows) + cardY,
//                 cardHitbox.width, cardHitbox.height},
//             (Vector2){cardHitbox.width/2,cardHitbox.height/2},
//             CheckCollisionPointRec(
//                 GetMousePosition(),
//                 (Rectangle){
//                     r.x + i%cols * (r.width-cardWidth)/(cols-1), r.y + i/cols * r.height/rows,
//                     cardWidth, r.height/rows}
//             ) ? (float)GetRandomValue(0,1000)/100.0f - 5 : 0,
//             ColorBrightness(WHITE, -0.8));
//     }
// }

static const float cardWidthToHeightRatio = 0.605119;
void fillRecWithAllCards(double startAngle, Texture2D cardTexture, Rectangle r);

// this is the old selector for cards in grid. now unfunctional as it relies on some static buffers (see above)
// inline static int scene_game_selectCard(void *arg)
// {
//     struct Game_client *g = arg;

//     Vector2 pos = GetMousePosition();
//     if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
//         for (int i = 0; i < g->player.hand.size; i++) {
//             if (true == CheckCollisionPointRec(pos,
//                     (Rectangle){ cardOrigins[i].x, cardOrigins[i].y, cardHitbox.width, cardHitbox.height }))
//                 return i;
//         }
//     }
//     return -1;
// }
