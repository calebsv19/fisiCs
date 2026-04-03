typedef union SDL_Event SDL_Event;

extern int SDL_PollEvent(SDL_Event *event);

int main(void) {
    return (SDL_PollEvent((SDL_Event *)0) >= -1) ? 0 : 5;
}
