#include <raylib.h>
#include <iostream>
#include <deque>

using namespace std;

const int screenWidth = 750;
const int screenHeight = 750;

int main()
{
    // Initialization
    InitWindow(screenWidth, screenHeight, "Retro Snake");
    SetTargetFPS(60); 
   
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Event Handling

        // Updating Positions

        // Drawing

        BeginDrawing();
        ClearBackground(GREEN); 
       
        EndDrawing();

    }
    CloseWindow();
    return 0;
}
