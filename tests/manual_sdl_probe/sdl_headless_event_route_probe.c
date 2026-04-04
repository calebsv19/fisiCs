#include <SDL2/SDL.h>
#include <stdio.h>

typedef struct {
    int x;
    int y;
    int keymask;
    int click_count;
    int motion_manhattan;
    int resize_count;
    int view_w;
    int view_h;
    int user_code_sum;
    int routed_events;
} RouteState;

static int abs_i(int v) { return v < 0 ? -v : v; }

static int push_event_checked(SDL_Event* ev, const char* label) {
    if (SDL_PushEvent(ev) != 1) {
        fprintf(stderr, "SDL_PushEvent failed for %s: %s\n", label, SDL_GetError());
        return 0;
    }
    return 1;
}

static int push_marker(Sint32 code) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_USEREVENT;
    ev.user.code = code;
    return push_event_checked(&ev, "user marker");
}

static int push_key_event(Uint32 type, SDL_Keycode key) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = type;
    ev.key.type = type;
    ev.key.state = (type == SDL_KEYDOWN) ? SDL_PRESSED : SDL_RELEASED;
    ev.key.repeat = 0;
    ev.key.keysym.sym = key;
    return push_event_checked(&ev, "key event");
}

static int push_motion_event(int x, int y, int xrel, int yrel) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_MOUSEMOTION;
    ev.motion.type = SDL_MOUSEMOTION;
    ev.motion.state = 0;
    ev.motion.x = x;
    ev.motion.y = y;
    ev.motion.xrel = xrel;
    ev.motion.yrel = yrel;
    return push_event_checked(&ev, "mouse motion");
}

static int push_button_event(Uint8 button, int x, int y) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_MOUSEBUTTONDOWN;
    ev.button.type = SDL_MOUSEBUTTONDOWN;
    ev.button.state = SDL_PRESSED;
    ev.button.button = button;
    ev.button.x = x;
    ev.button.y = y;
    return push_event_checked(&ev, "mouse button");
}

static int push_resize_event(int w, int h) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_WINDOWEVENT;
    ev.window.type = SDL_WINDOWEVENT;
    ev.window.event = SDL_WINDOWEVENT_RESIZED;
    ev.window.data1 = w;
    ev.window.data2 = h;
    return push_event_checked(&ev, "window resize");
}

static int push_user_code_event(Sint32 code) {
    SDL_Event ev;
    SDL_zero(ev);
    ev.type = SDL_USEREVENT;
    ev.user.type = SDL_USEREVENT;
    ev.user.code = code;
    return push_event_checked(&ev, "user code");
}

static void drain_event_queue(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        /* Drop startup/system events before deterministic routing. */
    }
}

static void route_event(RouteState* state, const SDL_Event* ev) {
    if (!state || !ev) return;
    switch (ev->type) {
        case SDL_KEYDOWN:
            if (ev->key.keysym.sym == SDLK_LEFT) {
                state->x -= 5;
                state->keymask |= 0x1;
                state->routed_events++;
            } else if (ev->key.keysym.sym == SDLK_UP) {
                state->y -= 7;
                state->keymask |= 0x2;
                state->routed_events++;
            }
            break;
        case SDL_KEYUP:
            if (ev->key.keysym.sym == SDLK_LEFT) {
                state->keymask &= ~0x1;
                state->routed_events++;
            } else if (ev->key.keysym.sym == SDLK_UP) {
                state->keymask &= ~0x2;
                state->routed_events++;
            }
            break;
        case SDL_MOUSEMOTION:
            state->x += ev->motion.xrel;
            state->y += ev->motion.yrel;
            state->motion_manhattan += abs_i(ev->motion.xrel) + abs_i(ev->motion.yrel);
            state->routed_events++;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (ev->button.button == SDL_BUTTON_LEFT) {
                state->click_count++;
                state->routed_events++;
            }
            break;
        case SDL_WINDOWEVENT:
            if (ev->window.event == SDL_WINDOWEVENT_RESIZED) {
                state->view_w = ev->window.data1;
                state->view_h = ev->window.data2;
                state->resize_count++;
                state->routed_events++;
            }
            break;
        case SDL_USEREVENT:
            state->user_code_sum += (int)ev->user.code;
            state->routed_events++;
            break;
        default:
            break;
    }
}

int main(void) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_PixelFormat* fmt = NULL;
    SDL_Rect one_px = {320, 180, 1, 1};
    Uint32 pixel = 0;
    Uint8 got_r = 0, got_g = 0, got_b = 0, got_a = 0;
    Uint8 want_r = 0, want_g = 0, want_b = 0, want_a = 255;
    RouteState state;
    SDL_Event ev;
    int started = 0;
    int ended = 0;
    int poll_guard = 0;

    state.x = 100;
    state.y = 50;
    state.keymask = 0;
    state.click_count = 0;
    state.motion_manhattan = 0;
    state.resize_count = 0;
    state.view_w = 640;
    state.view_h = 360;
    state.user_code_sum = 0;
    state.routed_events = 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("fisics-sdl-headless-route-probe",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              640,
                              360,
                              SDL_WINDOW_HIDDEN);
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

    drain_event_queue();

    if (!push_marker(111) ||
        !push_key_event(SDL_KEYDOWN, SDLK_LEFT) ||
        !push_key_event(SDL_KEYDOWN, SDLK_UP) ||
        !push_motion_event(320, 180, 4, -3) ||
        !push_button_event(SDL_BUTTON_LEFT, 42, 24) ||
        !push_resize_event(800, 600) ||
        !push_user_code_event(42) ||
        !push_key_event(SDL_KEYUP, SDLK_LEFT) ||
        !push_key_event(SDL_KEYUP, SDLK_UP) ||
        !push_marker(999)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    while (poll_guard < 512 && SDL_PollEvent(&ev)) {
        poll_guard++;
        if (ev.type == SDL_USEREVENT && ev.user.code == 111) {
            started = 1;
            continue;
        }
        if (!started) continue;
        if (ev.type == SDL_USEREVENT && ev.user.code == 999) {
            ended = 1;
            break;
        }
        route_event(&state, &ev);
    }

    if (!ended) {
        fprintf(stderr, "Did not observe end marker in event queue\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 5;
    }

    if (state.x != 99 || state.y != 40 || state.keymask != 0 ||
        state.click_count != 1 || state.motion_manhattan != 7 ||
        state.resize_count != 1 || state.view_w != 800 || state.view_h != 600 ||
        state.user_code_sum != 42 || state.routed_events != 8) {
        fprintf(stderr,
                "Route mismatch x=%d y=%d keymask=%d clicks=%d motion=%d resize=%d view=%dx%d user=%d routed=%d\n",
                state.x, state.y, state.keymask, state.click_count, state.motion_manhattan,
                state.resize_count, state.view_w, state.view_h, state.user_code_sum, state.routed_events);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 6;
    }

    want_r = (Uint8)((state.x + (5 * state.click_count) + state.user_code_sum) & 0xFF);
    want_g = (Uint8)((state.y + state.motion_manhattan + (3 * state.resize_count)) & 0xFF);
    want_b = (Uint8)(((state.routed_events * 17) + (state.view_w / 100) + (state.view_h / 100)) & 0xFF);

    if (SDL_SetRenderDrawColor(renderer, want_r, want_g, want_b, want_a) != 0 ||
        SDL_RenderClear(renderer) != 0) {
        fprintf(stderr, "Render clear failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 7;
    }
    SDL_RenderPresent(renderer);

    if (SDL_RenderReadPixels(renderer, &one_px, SDL_PIXELFORMAT_ARGB8888, &pixel, (int)sizeof(pixel)) != 0) {
        fprintf(stderr, "SDL_RenderReadPixels failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 8;
    }

    fmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    if (!fmt) {
        fprintf(stderr, "SDL_AllocFormat failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 9;
    }

    SDL_GetRGBA(pixel, fmt, &got_r, &got_g, &got_b, &got_a);
    SDL_FreeFormat(fmt);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (got_r != want_r || got_g != want_g || got_b != want_b || got_a != want_a) {
        fprintf(stderr, "Pixel mismatch want=(%u,%u,%u,%u) got=(%u,%u,%u,%u)\n",
                (unsigned)want_r, (unsigned)want_g, (unsigned)want_b, (unsigned)want_a,
                (unsigned)got_r, (unsigned)got_g, (unsigned)got_b, (unsigned)got_a);
        return 10;
    }

    printf("ROUTE_OK x=%d y=%d keymask=%d clicks=%d motion=%d resize=%d view=%dx%d user=%d routed=%d color=(%u,%u,%u,%u)\n",
           state.x, state.y, state.keymask, state.click_count, state.motion_manhattan,
           state.resize_count, state.view_w, state.view_h, state.user_code_sum, state.routed_events,
           ((unsigned)got_r) & 0xFFu, ((unsigned)got_g) & 0xFFu,
           ((unsigned)got_b) & 0xFFu, ((unsigned)got_a) & 0xFFu);
    return 0;
}
