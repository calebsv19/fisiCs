#include <SDL2/SDL.h>
#include <stdio.h>

void abi_probe_consume_color(SDL_Color c);

int main(void) {
    SDL_Color white = {255, 255, 255, 255};
    printf("MAIN_COLOR r=%u g=%u b=%u a=%u\n",
           (unsigned)white.r, (unsigned)white.g, (unsigned)white.b, (unsigned)white.a);
    abi_probe_consume_color(white);
    return 0;
}
