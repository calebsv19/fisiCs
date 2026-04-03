#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Event in_ev;
    SDL_Event out_ev;
    int pushed;
    int got;

    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        return 1;
    }

    SDL_zero(in_ev);
    in_ev.type = SDL_USEREVENT;
    in_ev.user.code = 77;

    pushed = SDL_PushEvent(&in_ev);
    if (pushed != 1) {
        SDL_Quit();
        return 2;
    }

    SDL_zero(out_ev);
    got = SDL_PollEvent(&out_ev);
    SDL_Quit();
    if (got != 1 || out_ev.type != SDL_USEREVENT || out_ev.user.code != 77) {
        return 3;
    }

    printf("sdl_event_roundtrip_ok\n");
    return 0;
}
