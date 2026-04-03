#include <SDL2/SDL.h>

int main(void) {
    Uint32 dst[4] = {0, 0, 0, 0};
    Uint32 src[4] = {1, 2, 3, 4};
    SDL_memcpy4(dst, src, 4);
    return (SDL_fabsf(-1.0f) > 0.5f && dst[0] == 1u && dst[3] == 4u) ? 0 : 2;
}
