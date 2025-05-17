#include "raylib.h"
#include "clock.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    if (!Initialize(argc, argv)) // Includes InitWindow call
        return 1;
    
    while (!WindowShouldClose())
        Loop();
    
    Uninitialize(); // Includes CloseWindow call
    return 0;
}