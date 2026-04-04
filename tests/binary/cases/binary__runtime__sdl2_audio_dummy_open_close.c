#include <SDL2/SDL.h>
#include <stdio.h>

static void audio_callback(void* userdata, Uint8* stream, int len) {
    int* calls = (int*)userdata;
    (void)stream;
    (void)len;
    if (calls) {
        (*calls)++;
    }
}

int main(void) {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    SDL_AudioDeviceID device;
    int callback_calls = 0;

    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        return 1;
    }

    SDL_zero(desired);
    desired.freq = 22050;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 256;
    desired.callback = audio_callback;
    desired.userdata = &callback_calls;

    device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (device == 0) {
        SDL_Quit();
        return 2;
    }

    SDL_PauseAudioDevice(device, 0);
    SDL_Delay(20);
    SDL_PauseAudioDevice(device, 1);

    if (obtained.freq <= 0 || obtained.channels == 0) {
        SDL_CloseAudioDevice(device);
        SDL_Quit();
        return 3;
    }

    SDL_CloseAudioDevice(device);
    SDL_Quit();
    printf("sdl_audio_dummy_open_close_ok\n");
    return 0;
}
