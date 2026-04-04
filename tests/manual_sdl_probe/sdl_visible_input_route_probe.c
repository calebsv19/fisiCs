#include <SDL2/SDL.h>
#include <stdio.h>

typedef struct {
    int key_down;
    int key_up;
    int text_input;
    int mouse_motion;
    int mouse_button;
    int mouse_wheel;
    int quit_events;
    int last_x;
    int last_y;
} InputState;

static Uint32 parse_wait_ms(const char* key, Uint32 fallback_ms, Uint32 max_ms) {
    const char* raw = SDL_getenv(key);
    const char* p = raw;
    unsigned long v = 0;
    if (!raw || !*raw) return fallback_ms;
    while (*p) {
        if (*p < '0' || *p > '9') return fallback_ms;
        v = (v * 10ul) + (unsigned long)(*p - '0');
        if (v > (unsigned long)max_ms) return fallback_ms;
        p++;
    }
    if (v == 0ul) return fallback_ms;
    return (Uint32)v;
}

static int parse_require_input(const char* key, int fallback) {
    const char* raw = SDL_getenv(key);
    if (!raw || !*raw) return fallback;
    if ((raw[0] == '0' && raw[1] == '\0') || raw[0] == 'n' || raw[0] == 'N') return 0;
    if ((raw[0] == '1' && raw[1] == '\0') || raw[0] == 'y' || raw[0] == 'Y') return 1;
    return fallback;
}

int main(void) {
    const Uint32 max_ms = parse_wait_ms("FISICS_SDL_INPUT_MAX_MS", 12000u, 120000u);
    const int require_input = parse_require_input("FISICS_SDL_REQUIRE_INPUT", 1);
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Rect one_px = {400, 240, 1, 1};
    SDL_Rect cursor_box = {380, 220, 40, 40};
    SDL_PixelFormat* fmt = NULL;
    SDL_Event ev;
    InputState state;
    Uint32 start_ticks = 0;
    Uint32 now_ticks = 0;
    Uint32 pixel = 0;
    Uint8 got_r = 0, got_g = 0, got_b = 0, got_a = 0;
    int running = 1;
    int interaction_score = 0;

    state.key_down = 0;
    state.key_up = 0;
    state.text_input = 0;
    state.mouse_motion = 0;
    state.mouse_button = 0;
    state.mouse_wheel = 0;
    state.quit_events = 0;
    state.last_x = 400;
    state.last_y = 240;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("fisics-sdl-input-route-probe",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              800,
                              480,
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

    SDL_StartTextInput();
    start_ticks = SDL_GetTicks();

    printf("INPUT_INFO driver=%s max_ms=%u require_input=%d\n",
           SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "<none>",
           (unsigned)max_ms,
           require_input);
    printf("INPUT_HINT move mouse, click, type, press ESC to quit early.\n");

    while (running) {
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    state.quit_events++;
                    running = 0;
                    break;
                case SDL_KEYDOWN:
                    state.key_down++;
                    if (ev.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    }
                    break;
                case SDL_KEYUP:
                    state.key_up++;
                    break;
                case SDL_TEXTINPUT:
                    state.text_input++;
                    break;
                case SDL_MOUSEMOTION:
                    state.mouse_motion++;
                    state.last_x = ev.motion.x;
                    state.last_y = ev.motion.y;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    state.mouse_button++;
                    state.last_x = ev.button.x;
                    state.last_y = ev.button.y;
                    break;
                case SDL_MOUSEWHEEL:
                    state.mouse_wheel += (ev.wheel.y >= 0) ? ev.wheel.y : -ev.wheel.y;
                    break;
                default:
                    break;
            }
        }

        now_ticks = SDL_GetTicks();
        if ((Uint32)(now_ticks - start_ticks) >= max_ms) {
            running = 0;
        }

        cursor_box.x = state.last_x - (cursor_box.w / 2);
        cursor_box.y = state.last_y - (cursor_box.h / 2);

        {
            Uint8 bg_r = (Uint8)(((state.key_down * 23) + (state.mouse_button * 41)) & 0xFF);
            Uint8 bg_g = (Uint8)(((state.mouse_motion * 7) + (state.mouse_wheel * 55)) & 0xFF);
            Uint8 bg_b = (Uint8)(((state.text_input * 31) + ((now_ticks - start_ticks) / 50u)) & 0xFF);
            if (SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255) != 0 ||
                SDL_RenderClear(renderer) != 0) {
                fprintf(stderr, "Render clear failed: %s\n", SDL_GetError());
                SDL_StopTextInput();
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 4;
            }
        }

        if (SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255) != 0 ||
            SDL_RenderDrawRect(renderer, &cursor_box) != 0) {
            fprintf(stderr, "Render draw rect failed: %s\n", SDL_GetError());
            SDL_StopTextInput();
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 5;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (SDL_RenderReadPixels(renderer, &one_px, SDL_PIXELFORMAT_ARGB8888, &pixel, (int)sizeof(pixel)) != 0) {
        fprintf(stderr, "SDL_RenderReadPixels failed: %s\n", SDL_GetError());
        SDL_StopTextInput();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 6;
    }

    fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    if (!fmt) {
        fprintf(stderr, "SDL_AllocFormat failed: %s\n", SDL_GetError());
        SDL_StopTextInput();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 7;
    }

    SDL_GetRGBA(pixel, fmt, &got_r, &got_g, &got_b, &got_a);
    SDL_FreeFormat(fmt);

    SDL_StopTextInput();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    interaction_score = state.key_down + state.key_up + state.text_input +
                        state.mouse_motion + state.mouse_button + state.mouse_wheel;
    if (require_input && interaction_score == 0) {
        fprintf(stderr, "INPUT_FAIL no input events captured (set FISICS_SDL_REQUIRE_INPUT=0 to bypass)\n");
        return 8;
    }

    printf("INPUT_OK keys_down=%d keys_up=%d text=%d motion=%d buttons=%d wheel=%d quit=%d last=(%d,%d) center_color=(%u,%u,%u,%u)\n",
           state.key_down, state.key_up, state.text_input, state.mouse_motion,
           state.mouse_button, state.mouse_wheel, state.quit_events,
           state.last_x, state.last_y,
           ((unsigned)got_r) & 0xFFu, ((unsigned)got_g) & 0xFFu,
           ((unsigned)got_b) & 0xFFu, ((unsigned)got_a) & 0xFFu);
    return 0;
}
