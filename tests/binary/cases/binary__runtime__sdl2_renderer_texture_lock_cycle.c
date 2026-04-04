#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    void* pixels = NULL;
    int pitch = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }

    window = SDL_CreateWindow("renderer_texture",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              16,
                              16,
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

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING, 2, 2);
    if (!texture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    if (SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch) != 0 || !pixels || pitch <= 0) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 5;
    }

    ((unsigned int*)pixels)[0] = 0x11223344u;
    ((unsigned int*)pixels)[1] = 0x55667788u;
    SDL_UnlockTexture(texture);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("sdl_renderer_texture_lock_cycle_ok\n");
    return 0;
}
