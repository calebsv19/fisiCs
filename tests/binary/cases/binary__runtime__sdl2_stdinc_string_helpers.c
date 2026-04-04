#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    char buf[32];
    size_t n1;
    size_t n2;
    int cmp;
    size_t len;

    SDL_memset(buf, 0, sizeof(buf));
    n1 = SDL_strlcpy(buf, "alpha", sizeof(buf));
    n2 = SDL_strlcat(buf, "-beta", sizeof(buf));
    cmp = SDL_strcmp(buf, "alpha-beta");
    len = SDL_strlen(buf);

    if (n1 != 5u || n2 != 10u || cmp != 0 || len != 10u) {
        return 1;
    }

    printf("sdl_stdinc_string_helpers_ok buf=%s len=%u\n", buf, (unsigned)len);
    return 0;
}
