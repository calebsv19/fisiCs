#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

static int sample_first_opaque_rgba(SDL_Surface* src, unsigned* out_r, unsigned* out_g, unsigned* out_b, unsigned* out_a) {
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA8888, 0);
    if (!conv) {
        return 0;
    }

    unsigned found = 0;
    Uint8 r = 0, g = 0, b = 0, a = 0;
    Uint32* px = (Uint32*)conv->pixels;
    int count = conv->w * conv->h;
    for (int i = 0; i < count; ++i) {
        SDL_GetRGBA(px[i], conv->format, &r, &g, &b, &a);
        if (a > 0) {
            found = 1;
            break;
        }
    }

    if (found) {
        *out_r = (unsigned)r;
        *out_g = (unsigned)g;
        *out_b = (unsigned)b;
        *out_a = (unsigned)a;
    }
    SDL_FreeSurface(conv);
    return (int)found;
}

int main(void) {
    if (SDL_Init(0) != 0) {
        printf("SDL_INIT_FAIL %s\n", SDL_GetError());
        return 2;
    }
    if (TTF_Init() != 0) {
        printf("TTF_INIT_FAIL %s\n", TTF_GetError());
        SDL_Quit();
        return 3;
    }

    const char* font_path = "../line_drawing/include/fonts/Lato/Lato-Regular.ttf";
    TTF_Font* f = TTF_OpenFont(font_path, 16);
    if (!f) {
        printf("FONT_OPEN_FAIL %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 4;
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* s = TTF_RenderText_Blended(f, "TEXT", white);
    if (!s) {
        printf("RENDER_FAIL %s\n", TTF_GetError());
        TTF_CloseFont(f);
        TTF_Quit();
        SDL_Quit();
        return 5;
    }

    unsigned r = 0, g = 0, b = 0, a = 0;
    int ok = sample_first_opaque_rgba(s, &r, &g, &b, &a);
    if (!ok) {
        printf("NO_OPAQUE_PIXEL\n");
    } else {
        printf("TTF_RGBA r=%u g=%u b=%u a=%u\n", r, g, b, a);
    }

    SDL_FreeSurface(s);
    TTF_CloseFont(f);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
