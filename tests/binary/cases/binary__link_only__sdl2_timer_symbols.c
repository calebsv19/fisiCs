typedef unsigned long long Uint64;

extern Uint64 SDL_GetPerformanceFrequency(void);
extern Uint64 SDL_GetPerformanceCounter(void);

int main(void) {
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 start = SDL_GetPerformanceCounter();
    Uint64 end = SDL_GetPerformanceCounter();
    return (freq > 0 && end >= start) ? 0 : 4;
}
