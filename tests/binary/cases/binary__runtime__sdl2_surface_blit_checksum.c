#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Surface* src = SDL_CreateRGBSurfaceWithFormat(0, 4, 4, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, 4, 4, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 src_color;
    Uint32* pixels;
    int i;

    if (!src || !dst) {
        if (src) SDL_FreeSurface(src);
        if (dst) SDL_FreeSurface(dst);
        return 1;
    }

    src_color = SDL_MapRGBA(src->format, 0x7f, 0x20, 0x40, 0xff);
    if (SDL_FillRect(src, NULL, src_color) != 0 || SDL_FillRect(dst, NULL, 0) != 0) {
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
        return 2;
    }

    if (SDL_BlitSurface(src, NULL, dst, NULL) != 0) {
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
        return 3;
    }

    pixels = (Uint32*)dst->pixels;
    if (!pixels) {
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
        return 4;
    }

    for (i = 0; i < 16; ++i) {
        if (pixels[i] != src_color) {
            SDL_FreeSurface(src);
            SDL_FreeSurface(dst);
            return 5;
        }
    }

    SDL_FreeSurface(src);
    SDL_FreeSurface(dst);
    printf("sdl_surface_blit_checksum_ok\n");
    return 0;
}
