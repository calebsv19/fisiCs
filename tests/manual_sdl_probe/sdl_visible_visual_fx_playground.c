#include <SDL2/SDL.h>
#include <stdio.h>

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

int main(void) {
    const int width = 900;
    const int height = 540;
    const Uint32 max_ms = parse_wait_ms("FISICS_SDL_VISUAL_FX_MS", 16000u, 180000u);
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event ev;
    Uint32 start_ticks = 0;
    Uint32 now_ticks = 0;
    int running = 1;
    int paused = 0;
    int theme = 0;
    int speed_mode = 0;
    int toggles = 0;
    int mouse_x = width / 2;
    int mouse_y = height / 2;
    int mouse_held = 0;
    int checksum = 0;
    int frame_count = 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("fisics-sdl-visual-fx-playground",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width,
                              height,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }

    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);

    printf("VISUAL_FX_INFO driver=%s max_ms=%u\n",
           SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "<none>",
           (unsigned)max_ms);
    printf("VISUAL_FX_HINT keys=[SPACE pause][TAB theme][S speed][ESC quit] mouse=[move/hold]\n");

    start_ticks = SDL_GetTicks();
    while (running) {
        SDL_Rect mini = {width - 230, 10, 220, 130};
        int i = 0;

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_KEYDOWN:
                    if (ev.key.keysym.sym == SDLK_ESCAPE) {
                        running = 0;
                    } else if (ev.key.keysym.sym == SDLK_SPACE) {
                        paused = !paused;
                        toggles++;
                    } else if (ev.key.keysym.sym == SDLK_TAB) {
                        theme = (theme + 1) % 3;
                        toggles++;
                    } else if (ev.key.keysym.sym == SDLK_s) {
                        speed_mode = (speed_mode + 1) % 3;
                        toggles++;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    mouse_x = ev.motion.x;
                    mouse_y = ev.motion.y;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.button == SDL_BUTTON_LEFT) mouse_held = 1;
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (ev.button.button == SDL_BUTTON_LEFT) mouse_held = 0;
                    break;
                default:
                    break;
            }
        }

        now_ticks = SDL_GetTicks();
        if ((Uint32)(now_ticks - start_ticks) >= max_ms) {
            running = 0;
        }

        if (!paused) frame_count++;

        {
            Uint8 br = (theme == 0) ? 8 : (theme == 1 ? 20 : 6);
            Uint8 bg = (theme == 0) ? 12 : (theme == 1 ? 6 : 20);
            Uint8 bb = (theme == 0) ? 22 : (theme == 1 ? 18 : 10);
            SDL_SetRenderDrawColor(renderer, br, bg, bb, 255);
            SDL_RenderClear(renderer);
        }

        for (i = 0; i < height; i += 18) {
            Uint8 g = (Uint8)(16 + ((i * 7 + frame_count) % 70));
            Uint8 b = (Uint8)(20 + ((i * 5 + frame_count * 2) % 65));
            SDL_Rect band = {0, i, width, 1};
            SDL_SetRenderDrawColor(renderer, 8, g, b, 255);
            SDL_RenderFillRect(renderer, &band);
        }

        SDL_SetRenderDrawColor(renderer, 26, 32, 45, 220);
        SDL_RenderFillRect(renderer, &mini);
        SDL_SetRenderDrawColor(renderer, 215, 226, 240, 255);
        SDL_RenderDrawRect(renderer, &mini);

        for (i = 0; i < 20; i++) {
            int speed = (speed_mode == 0) ? 2 : (speed_mode == 1 ? 4 : 6);
            int x = (i * 43 + frame_count * speed + i * i) % width;
            int y = (i * 71 + frame_count * (speed + 1) + i * 13) % height;
            int wobble_x = (mouse_held ? ((mouse_x - x) / 12) : 0);
            int wobble_y = (mouse_held ? ((mouse_y - y) / 12) : 0);
            SDL_Rect r = {
                x - 8 + wobble_x,
                y - 8 + wobble_y,
                16 + (i % 4),
                16 + (i % 5)
            };
            Uint8 rr = (Uint8)(40 + ((i * 19 + frame_count * 2) % 200));
            Uint8 gg = (Uint8)(30 + ((i * 23 + frame_count * 3) % 210));
            Uint8 bb = (Uint8)(50 + ((i * 29 + frame_count * 5) % 190));
            int mx = mini.x + ((r.x < 0 ? 0 : (r.x >= width ? width - 1 : r.x)) * mini.w) / width;
            int my = mini.y + ((r.y < 0 ? 0 : (r.y >= height ? height - 1 : r.y)) * mini.h) / height;

            SDL_SetRenderDrawColor(renderer, rr, gg, bb, 220);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
            SDL_RenderDrawRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, rr, gg, bb, 255);
            SDL_RenderDrawPoint(renderer, mx, my);

            checksum ^= (r.x * 31) ^ (r.y * 17) ^ (rr * 13) ^ (gg * 7) ^ (bb * 5);
        }

        if (mouse_held) {
            SDL_Rect hline = {mouse_x - 12, mouse_y - 1, 25, 3};
            SDL_Rect vline = {mouse_x - 1, mouse_y - 12, 3, 25};
            SDL_SetRenderDrawColor(renderer, 255, 220, 130, 240);
            SDL_RenderFillRect(renderer, &hline);
            SDL_RenderFillRect(renderer, &vline);
        }

        {
            SDL_Rect bar_bg = {10, height - 18, 260, 10};
            int fill_w = (int)((260u * (Uint32)(now_ticks - start_ticks)) / (max_ms ? max_ms : 1u));
            SDL_Rect bar_fg = {10, height - 18, fill_w > 260 ? 260 : fill_w, 10};
            SDL_SetRenderDrawColor(renderer, 24, 28, 38, 255);
            SDL_RenderFillRect(renderer, &bar_bg);
            SDL_SetRenderDrawColor(renderer, 90, 210, 180, 255);
            SDL_RenderFillRect(renderer, &bar_fg);
            SDL_SetRenderDrawColor(renderer, 225, 236, 250, 255);
            SDL_RenderDrawRect(renderer, &bar_bg);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    printf("VISUAL_FX_OK frames=%d toggles=%d theme=%d speed=%d checksum=%d mouse_held=%d\n",
           frame_count, toggles, theme, speed_mode, checksum, mouse_held);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
