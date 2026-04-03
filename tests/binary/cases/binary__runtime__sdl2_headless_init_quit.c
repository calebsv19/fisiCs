#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        return 1;
    }
    SDL_Quit();
    printf("sdl_headless_init_quit_ok\n");
    return 0;
}
