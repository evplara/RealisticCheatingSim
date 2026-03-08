#include <raylib.h>

#include "Config.h"
#include "Game.h"

int main()
{
    InitWindow(config::kScreenWidth, config::kScreenHeight, "Realistic Cheating Sim");
    SetTargetFPS(60);

    Game game{};
    game.Init();

    while (!WindowShouldClose() && !game.ShouldExit())
    {
        const float dt = GetFrameTime();
        game.Update(dt);

        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
