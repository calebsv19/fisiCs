#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 4, 4, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 color;
    Uint32* pixels;

    if (!surface) {
        return 1;
    }

    color = SDL_MapRGBA(surface->format, 0x11, 0x22, 0x33, 0x44);
    if (SDL_FillRect(surface, NULL, color) != 0) {
        SDL_FreeSurface(surface);
        return 2;
    }

    pixels = (Uint32*)surface->pixels;
    if (!pixels || pixels[0] != color) {
        SDL_FreeSurface(surface);
        return 3;
    }

    SDL_FreeSurface(surface);
    printf("sdl_surface_fill_readback_ok\n");
    return 0;
}
