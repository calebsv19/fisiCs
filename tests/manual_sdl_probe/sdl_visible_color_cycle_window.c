#include <SDL2/SDL.h>
#include <stdio.h>

typedef struct {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
} RGBA;

static Uint32 read_wait_ms_from_env(const char* key, Uint32 default_ms) {
    const char* raw = SDL_getenv(key);
    const char* p = NULL;
    unsigned long value = 0;
    if (!raw || !*raw) {
        return default_ms;
    }
    p = raw;
    while (*p) {
        if (*p < '0' || *p > '9') {
            return default_ms;
        }
        value = (value * 10ul) + (unsigned long)(*p - '0');
        if (value > 120000ul) {
            return default_ms;
        }
        p++;
    }
    if (value == 0ul) {
        return default_ms;
    }
    return (Uint32)value;
}

int main(void) {
    const RGBA colors[] = {
        {220, 30, 30, 255},
        {30, 180, 80, 255},
        {30, 90, 220, 255},
        {230, 200, 40, 255}
    };
    const Uint32 total_ms = read_wait_ms_from_env("FISICS_SDL_VISIBLE_CYCLE_MS", 7000u);
    const Uint32 frame_ms = 350u;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_PixelFormat* fmt = NULL;
    SDL_Rect center_px = {320, 180, 1, 1};
    SDL_Event evt;
    Uint32 start_ticks = 0;
    Uint32 now_ticks = 0;
    Uint32 pixel = 0;
    size_t index = 0;
    Uint8 got_r = 0;
    Uint8 got_g = 0;
    Uint8 got_b = 0;
    Uint8 got_a = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("fisics-sdl-visible-cycle-probe",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              640,
                              360,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }

    printf("VISIBLE_CYCLE_INFO driver=%s total_ms=%u frame_ms=%u colors=%u\n",
           SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "<none>",
           (unsigned)total_ms,
           (unsigned)frame_ms,
           (unsigned)(sizeof(colors) / sizeof(colors[0])));

    start_ticks = SDL_GetTicks();
    while (1) {
        const RGBA* c = &colors[index % (sizeof(colors) / sizeof(colors[0]))];
        now_ticks = SDL_GetTicks();
        if ((Uint32)(now_ticks - start_ticks) >= total_ms) {
            break;
        }

        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) {
                fprintf(stderr, "VISIBLE_CYCLE_QUIT received before timeout\n");
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 4;
            }
        }

        if (SDL_SetRenderDrawColor(renderer, c->r, c->g, c->b, c->a) != 0 ||
            SDL_RenderClear(renderer) != 0) {
            fprintf(stderr, "Render clear failed: %s\n", SDL_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 5;
        }

        SDL_RenderPresent(renderer);
        index++;
        SDL_Delay(frame_ms);
    }

    if (SDL_RenderReadPixels(renderer, &center_px, SDL_PIXELFORMAT_ARGB8888, &pixel, (int)sizeof(pixel)) != 0) {
        fprintf(stderr, "SDL_RenderReadPixels failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 6;
    }

    fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    if (!fmt) {
        fprintf(stderr, "SDL_AllocFormat failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 7;
    }

    SDL_GetRGBA(pixel, fmt, &got_r, &got_g, &got_b, &got_a);
    SDL_FreeFormat(fmt);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("VISIBLE_CYCLE_OK final_color=(%u,%u,%u,%u) frames=%u\n",
           ((unsigned)got_r) & 0xFFu,
           ((unsigned)got_g) & 0xFFu,
           ((unsigned)got_b) & 0xFFu,
           ((unsigned)got_a) & 0xFFu,
           (unsigned)index);
    return 0;
}
