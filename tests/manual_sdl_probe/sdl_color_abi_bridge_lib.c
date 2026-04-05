#include <SDL2/SDL.h>
#include <stdio.h>

void abi_probe_consume_color(SDL_Color c) {
    printf("LIB_COLOR r=%u g=%u b=%u a=%u\n",
           (unsigned)c.r, (unsigned)c.g, (unsigned)c.b, (unsigned)c.a);
}
