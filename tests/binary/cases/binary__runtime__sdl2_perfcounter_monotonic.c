#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 c0;
    Uint64 c1;

    if (freq == 0) {
        return 1;
    }

    c0 = SDL_GetPerformanceCounter();
    SDL_Delay(1);
    c1 = SDL_GetPerformanceCounter();
    if (c1 < c0) {
        return 2;
    }

    printf("sdl_perfcounter_monotonic_ok\n");
    return 0;
}
