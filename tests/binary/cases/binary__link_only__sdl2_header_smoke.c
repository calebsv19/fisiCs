/* Symbol-surface smoke without relying on SDL headers (complements real-header tests). */
extern const char *SDL_GetError(void);
extern void SDL_ClearError(void);

int main(void) {
    SDL_ClearError();
    return SDL_GetError() ? 0 : 1;
}
