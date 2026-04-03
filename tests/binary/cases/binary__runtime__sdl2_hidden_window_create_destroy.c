#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Window* window;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }

    window = SDL_CreateWindow("e3_hidden",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              16,
                              16,
                              SDL_WINDOW_HIDDEN);
    if (!window) {
        SDL_Quit();
        return 2;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("sdl_hidden_window_ok\n");
    return 0;
}
