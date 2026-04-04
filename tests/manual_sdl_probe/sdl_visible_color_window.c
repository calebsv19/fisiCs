#include <SDL2/SDL.h>
#include <stdio.h>

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
        if (value > 60000ul) {
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
    const Uint8 target_r = 20;
    const Uint8 target_g = 80;
    const Uint8 target_b = 220;
    const Uint8 target_a = 255;
    const Uint32 visible_ms = read_wait_ms_from_env("FISICS_SDL_VISIBLE_MS", 5000u);
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_PixelFormat* fmt = NULL;
    SDL_Rect one_px = {0, 0, 1, 1};
    SDL_Rect win_bounds = {0, 0, 0, 0};
    SDL_Event evt;
    Uint32 start_ticks = 0;
    Uint32 now_ticks = 0;
    Uint32 pixel = 0;
    Uint8 got_r = 0;
    Uint8 got_g = 0;
    Uint8 got_b = 0;
    Uint8 got_a = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("fisics-sdl-visible-probe",
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

    SDL_GetWindowPosition(window, &win_bounds.x, &win_bounds.y);
    SDL_GetWindowSize(window, &win_bounds.w, &win_bounds.h);
    printf("VISIBLE_INFO driver=%s pos=(%d,%d) size=%dx%d flags=0x%x wait_ms=%u\n",
           SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "<none>",
           win_bounds.x, win_bounds.y, win_bounds.w, win_bounds.h,
           (unsigned)SDL_GetWindowFlags(window), (unsigned)visible_ms);

    if (SDL_SetRenderDrawColor(renderer, target_r, target_g, target_b, target_a) != 0 ||
        SDL_RenderClear(renderer) != 0) {
        fprintf(stderr, "Render clear failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    SDL_RenderPresent(renderer);

    start_ticks = SDL_GetTicks();
    while (1) {
        now_ticks = SDL_GetTicks();
        if ((Uint32)(now_ticks - start_ticks) >= visible_ms) {
            break;
        }
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) {
                fprintf(stderr, "VISIBLE_QUIT received before timeout\n");
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 8;
            }
        }
        SDL_Delay(16);
    }

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

    printf("VISIBLE_OK color=(%u,%u,%u,%u)\n",
           ((unsigned)got_r) & 0xFFu, ((unsigned)got_g) & 0xFFu,
           ((unsigned)got_b) & 0xFFu, ((unsigned)got_a) & 0xFFu);
    return 0;
}
