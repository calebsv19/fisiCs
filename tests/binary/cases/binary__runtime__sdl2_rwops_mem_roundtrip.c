#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    unsigned char buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char out[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char payload[4] = {'A', 'B', 'C', 'D'};
    SDL_RWops* rw = SDL_RWFromMem(buffer, (int)sizeof(buffer));
    size_t written;
    size_t read_count;

    if (!rw) {
        return 1;
    }

    written = (size_t)SDL_RWwrite(rw, payload, 1, sizeof(payload));
    if (written != sizeof(payload)) {
        SDL_RWclose(rw);
        return 2;
    }

    if (SDL_RWseek(rw, 0, RW_SEEK_SET) < 0) {
        SDL_RWclose(rw);
        return 3;
    }

    read_count = (size_t)SDL_RWread(rw, out, 1, sizeof(payload));
    SDL_RWclose(rw);
    if (read_count != sizeof(payload) || memcmp(payload, out, sizeof(payload)) != 0) {
        return 4;
    }

    printf("sdl_rwops_mem_roundtrip_ok\n");
    return 0;
}
