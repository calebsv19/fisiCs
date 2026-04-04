#include <SDL2/SDL.h>
#include <stdio.h>

static int read_rgba_at(SDL_Surface* surface, int x, int y, Uint8* r, Uint8* g, Uint8* b, Uint8* a) {
    Uint8* row;
    Uint32 pixel;
    if (!surface || x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
        return 0;
    }
    row = (Uint8*)surface->pixels + y * surface->pitch + x * 4;
    SDL_memcpy(&pixel, row, sizeof(pixel));
    SDL_GetRGBA(pixel, surface->format, r, g, b, a);
    return 1;
}

static int pixel_eq(Uint8 r, Uint8 g, Uint8 b, Uint8 a, Uint8 er, Uint8 eg, Uint8 eb, Uint8 ea) {
    return r == er && g == eg && b == eb && a == ea;
}

int main(void) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Surface* capture = NULL;
    SDL_Rect clip = {8, 8, 48, 48};
    Uint8 r = 0, g = 0, b = 0, a = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }

    window = SDL_CreateWindow("drawline_clip",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              64,
                              64,
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

    if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 ||
        SDL_RenderClear(renderer) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 5;
    }

    if (SDL_RenderSetClipRect(renderer, &clip) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 6;
    }

    if (SDL_SetRenderDrawColor(renderer, 40, 200, 90, 255) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 7;
    }

    if (SDL_RenderDrawLine(renderer, -32, 24, 96, 24) != 0 ||
        SDL_RenderDrawLine(renderer, 24, -32, 24, 96) != 0 ||
        SDL_RenderDrawLine(renderer, 0, 0, 63, 63) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 8;
    }

    SDL_RenderPresent(renderer);

    capture = SDL_CreateRGBSurfaceWithFormat(0, 64, 64, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!capture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 9;
    }

    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA8888, capture->pixels, capture->pitch) != 0) {
        SDL_FreeSurface(capture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 10;
    }

    /* Inside clip: must be painted by one of the three lines. */
    if (!read_rgba_at(capture, 24, 24, &r, &g, &b, &a) ||
        !pixel_eq(r, g, b, a, 40, 200, 90, 255)) {
        SDL_FreeSurface(capture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 11;
    }

    if (!read_rgba_at(capture, 10, 10, &r, &g, &b, &a) ||
        !pixel_eq(r, g, b, a, 40, 200, 90, 255)) {
        SDL_FreeSurface(capture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 12;
    }

    /* Outside clip: must remain clear color. */
    if (!read_rgba_at(capture, 4, 24, &r, &g, &b, &a) ||
        !pixel_eq(r, g, b, a, 0, 0, 0, 255)) {
        SDL_FreeSurface(capture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 13;
    }

    if (!read_rgba_at(capture, 24, 4, &r, &g, &b, &a) ||
        !pixel_eq(r, g, b, a, 0, 0, 0, 255)) {
        SDL_FreeSurface(capture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 14;
    }

    SDL_FreeSurface(capture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("sdl_renderer_drawline_clip_readback_ok\n");
    return 0;
}
