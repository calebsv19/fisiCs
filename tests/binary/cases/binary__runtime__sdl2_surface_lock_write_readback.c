#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 4, 3, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 mapped_color;
    Uint32 readback_color;
    unsigned char* row_ptr;
    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    Uint8 a = 0;

    if (!surface) {
        return 1;
    }

    if (SDL_LockSurface(surface) != 0) {
        SDL_FreeSurface(surface);
        return 2;
    }

    mapped_color = SDL_MapRGBA(surface->format, 12, 34, 56, 78);
    row_ptr = (unsigned char*)surface->pixels + surface->pitch;
    ((Uint32*)row_ptr)[2] = mapped_color;
    readback_color = ((Uint32*)row_ptr)[2];
    SDL_UnlockSurface(surface);

    if (readback_color != mapped_color) {
        SDL_FreeSurface(surface);
        return 3;
    }

    SDL_GetRGBA(readback_color, surface->format, &r, &g, &b, &a);
    SDL_FreeSurface(surface);
    if (r != 12 || g != 34 || b != 56 || a != 78) {
        return 4;
    }

    printf("sdl_surface_lock_write_readback_ok\n");
    return 0;
}
