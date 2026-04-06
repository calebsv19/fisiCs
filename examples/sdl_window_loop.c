// SPDX-License-Identifier: Apache-2.0

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

int main(void) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event ev;
    bool running = true;
    int frame = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("fisiCs SDL Example",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              800,
                              480,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        {
            Uint8 r = (Uint8)((frame * 2) % 255);
            Uint8 g = (Uint8)((frame * 3) % 255);
            Uint8 b = (Uint8)((frame * 5) % 255);

            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        }

        frame++;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

