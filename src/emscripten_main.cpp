#include <emscripten/emscripten.h>
#include "clock.h"

extern "C" void loop_iter(void) { Loop(); }

int main(int argc, char** argv)
{
    if (!Initialize(argc, argv))
        return 1;

    emscripten_set_main_loop(loop_iter, 0, 1);
    return 0;
}