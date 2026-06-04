#include <fenv.h>

#pragma STDC FENV_ACCESS ON

int header_corpus_wave12_rounding_surface(void) {
    fenv_t env;
    int before = fegetround();

    if (feholdexcept(&env) != 0) {
        return before;
    }

    fesetround(FE_TONEAREST);
    feupdateenv(&env);
    return before;
}
