#include <fenv.h>
#include <stdio.h>

static int header_corpus_wave12_fenv_summary(void) {
    int original = fegetround();
    int mask = 0;
    int changed = 0;
    int restored = 0;

    if (original == -1) {
        return -1;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return -2;
    }
    if (feraiseexcept(FE_DIVBYZERO | FE_INEXACT) != 0) {
        return -3;
    }

    mask = fetestexcept(FE_DIVBYZERO | FE_INEXACT | FE_INVALID);
    if (fesetround(FE_DOWNWARD) != 0) {
        return -4;
    }
    changed = fegetround() == FE_DOWNWARD ? 1 : 0;
    if (fesetround(original) != 0) {
        return -5;
    }
    restored = fegetround() == original ? 1 : 0;

    return ((mask & FE_DIVBYZERO) ? 1000 : 0) +
           ((mask & FE_INEXACT) ? 100 : 0) +
           ((mask & FE_INVALID) ? 10 : 0) +
           changed * 2 +
           restored;
}

int main(void) {
    int summary = header_corpus_wave12_fenv_summary();

    if (summary != 1103) {
        return 1;
    }

    printf("summary=%d\n", summary);
    return 0;
}
