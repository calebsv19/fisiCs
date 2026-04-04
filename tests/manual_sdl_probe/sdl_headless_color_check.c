#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    const Uint8 target_r = 12;
    const Uint8 target_g = 34;
    const Uint8 target_b = 56;
    const Uint8 target_a = 255;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_PixelFormat* fmt = NULL;
    SDL_Rect one_px = {0, 0, 1, 1};
    Uint32 pixel = 0;
    Uint8 got_r = 0;
    Uint8 got_g = 0;
    Uint8 got_b = 0;
    Uint8 got_a = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("headless-color-check",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              64,
                              64,
                              SDL_WINDOW_HIDDEN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }

    if (SDL_SetRenderDrawColor(renderer, target_r, target_g, target_b, target_a) != 0 ||
        SDL_RenderClear(renderer) != 0) {
        fprintf(stderr, "Render clear failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    SDL_RenderPresent(renderer);

    if (SDL_RenderReadPixels(renderer, &one_px, SDL_PIXELFORMAT_ARGB8888, &pixel, (int)sizeof(pixel)) != 0) {
        fprintf(stderr, "SDL_RenderReadPixels failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 5;
    }

    fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    if (!fmt) {
        fprintf(stderr, "SDL_AllocFormat failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 6;
    }

    SDL_GetRGBA(pixel, fmt, &got_r, &got_g, &got_b, &got_a);
    SDL_FreeFormat(fmt);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (got_r != target_r || got_g != target_g || got_b != target_b || got_a != target_a) {
        fprintf(stderr, "Color mismatch. expected=(%u,%u,%u,%u) got=(%u,%u,%u,%u)\n",
                ((unsigned)target_r) & 0xFFu, ((unsigned)target_g) & 0xFFu,
                ((unsigned)target_b) & 0xFFu, ((unsigned)target_a) & 0xFFu,
                ((unsigned)got_r) & 0xFFu, ((unsigned)got_g) & 0xFFu,
                ((unsigned)got_b) & 0xFFu, ((unsigned)got_a) & 0xFFu);
        return 7;
    }

    printf("HEADLESS_OK color=(%u,%u,%u,%u)\n",
           ((unsigned)got_r) & 0xFFu, ((unsigned)got_g) & 0xFFu,
           ((unsigned)got_b) & 0xFFu, ((unsigned)got_a) & 0xFFu);
    return 0;
}
