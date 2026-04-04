#include <SDL2/SDL.h>

extern int SDL_NonexistentSymbol_ForPolicy(void);

int main(void) {
    SDL_version v;
    SDL_VERSION(&v);
    return SDL_NonexistentSymbol_ForPolicy();
}
