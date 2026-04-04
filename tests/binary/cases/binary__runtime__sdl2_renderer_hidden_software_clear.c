#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Window* window;
    SDL_Renderer* renderer;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }

    window = SDL_CreateWindow("renderer_hidden",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              32,
                              32,
                              SDL_WINDOW_HIDDEN);
    if (!window) {
        SDL_Quit();
        return 2;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }

    if (SDL_SetRenderDrawColor(renderer, 10, 20, 30, 255) != 0 ||
        SDL_RenderClear(renderer) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    SDL_RenderPresent(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("sdl_renderer_hidden_software_clear_ok\n");
    return 0;
}
