typedef unsigned int Uint32;

extern int SDL_Init(Uint32 flags);
extern void SDL_Quit(void);

int main(void) {
    if (SDL_Init(0) != 0) {
        return 3;
    }
    SDL_Quit();
    return 0;
}
