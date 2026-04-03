#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    Uint32 t0 = SDL_GetTicks();
    SDL_Delay(2);
    {
        Uint32 t1 = SDL_GetTicks();
        if (t1 < t0) {
            return 3;
        }
    }
    printf("sdl_ticks_monotonic_ok\n");
    return 0;
}
