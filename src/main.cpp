#include "raylib.h"
#include "clock.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    if (!Initialize(argc, argv))
        return 1;
    
    while (!WindowShouldClose())
        Loop();
    
    Uninitialize();
    return 0;
}