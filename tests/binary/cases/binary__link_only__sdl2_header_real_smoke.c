#include <SDL2/SDL.h>

int main(void) {
    SDL_version version;
    SDL_VERSION(&version);
    return (version.major >= 2) ? 0 : 1;
}
