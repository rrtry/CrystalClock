#include "raylib.h"
#include "clock.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    Initialize(argc, argv);
    while (!WindowShouldClose())
        Loop();
    
    Uninitialize();
    return 0;
}