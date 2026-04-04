#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Event in_ev;
    SDL_Event out_ev;
    Uint32 event_type;
    int marker = 42;

    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        return 1;
    }

    event_type = SDL_RegisterEvents(1);
    if (event_type == (Uint32)-1) {
        SDL_Quit();
        return 2;
    }

    SDL_zero(in_ev);
    in_ev.type = event_type;
    in_ev.user.type = event_type;
    in_ev.user.code = 1234;
    in_ev.user.data1 = &marker;
    if (SDL_PushEvent(&in_ev) != 1) {
        SDL_Quit();
        return 3;
    }

    SDL_zero(out_ev);
    if (SDL_PollEvent(&out_ev) != 1) {
        SDL_Quit();
        return 4;
    }

    SDL_Quit();
    if (out_ev.type != event_type || out_ev.user.code != 1234 || out_ev.user.data1 != &marker) {
        return 5;
    }

    printf("sdl_register_events_roundtrip_ok\n");
    return 0;
}
