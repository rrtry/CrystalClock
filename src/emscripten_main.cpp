#include <emscripten/emscripten.h>
#include "clock.h"

extern "C"
{
    static bool is_running = false;

    void loop_iter(void)
    {
        Loop();
    }

    void start_loop(void)
    {
        if (!is_running)
        {
            emscripten_set_main_loop(&loop_iter, 0, 1);
            is_running = true;
        }
    }

    void stop_loop(void)
    {
        if (is_running) 
        {
            emscripten_cancel_main_loop();
            is_running = false;
        }
    }
}

int main(int argc, char** argv)
{
    if (!Initialize(argc, argv))
        return 1;

    start_loop();
    return 0;
}