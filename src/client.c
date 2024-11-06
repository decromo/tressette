#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "client/client.h"
#include "client/client_plug.h"
#include "common/platform.h"

#include <raylib.h>

int main(int argc, char **argv)
{
    SetConfigFlags(FLAG_WINDOW_TOPMOST | FLAG_WINDOW_HIGHDPI | FLAG_BORDERLESS_WINDOWED_MODE);
    InitWindow(800, 600, "tressette");
    SetTargetFPS(60);
    // SetWindowMonitor(0);
    SetWindowPosition(5000, 0);

    while (!WindowShouldClose()) {
        int ret;
        struct Game_client game = {
            .addrinfo_found = false,
            .connection_established = false,
            .lost_connection = false,
            .game_over = false,
            .game_aborted = false
        };
        while (true) {

            plug_open();
            plug_init();

            ret = client_main(&game);

            plug_close();

            if (ret != 0) break;
        }
        if (WindowShouldClose() || ret == -1) {
            break;
        }
        // loop over and start a new game
    }

    printf("Thanks for playing ~~~!\n");

    CloseWindow();

    return 0;
}