#include <fenv.h>

int header_corpus_wave36_fenv_bridge_score(void) {
    fenv_t env;
    fexcept_t flags;
    int score = 0;
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_INVALID);
    score += fegetenv(&env) == 0 ? 3 : 0;
    score += fegetexceptflag(&flags, FE_INVALID) == 0 ? 5 : 0;
    feclearexcept(FE_ALL_EXCEPT);
    fesetexceptflag(&flags, FE_INVALID);
    score += (fetestexcept(FE_INVALID) & FE_INVALID) ? 7 : 0;
    fesetenv(&env);
    return score;
}
