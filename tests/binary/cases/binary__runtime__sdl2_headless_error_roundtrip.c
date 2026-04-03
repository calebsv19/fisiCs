#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    SDL_ClearError();
    SDL_SetError("wave_e2_error_marker");
    {
        const char* err = SDL_GetError();
        if (!err || strcmp(err, "wave_e2_error_marker") != 0) {
            return 2;
        }
    }
    printf("sdl_error_roundtrip_ok\n");
    return 0;
}
