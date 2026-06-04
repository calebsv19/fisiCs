#include <fenv.h>

static int header_corpus_wave36_env_surface(int mode) {
    fenv_t env;
    const fenv_t *default_env = FE_DFL_ENV;
    int before = fegetround();
    int saved = fegetenv(&env);
    if (mode) {
        fesetround(FE_TOWARDZERO);
    }
    fesetenv(default_env);
    fesetenv(&env);
    return saved + (before == fegetround() ? 1 : 0);
}

int main(void) {
    return header_corpus_wave36_env_surface(1) == 0;
}
