#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_PixelFormat* fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    Uint32 pixel;
    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    Uint8 a = 0;

    if (!fmt) {
        return 1;
    }

    pixel = SDL_MapRGBA(fmt, 12, 34, 56, 78);
    SDL_GetRGBA(pixel, fmt, &r, &g, &b, &a);
    SDL_FreeFormat(fmt);

    if (r != 12 || g != 34 || b != 56 || a != 78) {
        return 2;
    }

    printf("sdl_pixel_format_roundtrip_ok rgba=%u,%u,%u,%u\n",
           (unsigned)r, (unsigned)g, (unsigned)b, (unsigned)a);
    return 0;
}
