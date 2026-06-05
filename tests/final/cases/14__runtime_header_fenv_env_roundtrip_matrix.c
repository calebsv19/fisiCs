#include <fenv.h>
#include <stdio.h>

int main(void) {
    fenv_t original;
    fenv_t marked;
    int raised;
    int cleared;
    int restored;
    int round_original;
    int round_changed;
    int round_restored;

    if (fegetenv(&original) != 0) {
        return 1;
    }
    round_original = fegetround();
    if (round_original == -1) {
        return 2;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    if (fesetround(FE_TOWARDZERO) != 0) {
        return 4;
    }
    round_changed = fegetround() == FE_TOWARDZERO ? 1 : 0;
    if (feraiseexcept(FE_INVALID | FE_INEXACT) != 0) {
        return 5;
    }
    if (fegetenv(&marked) != 0) {
        return 6;
    }
    raised = fetestexcept(FE_INVALID | FE_INEXACT | FE_DIVBYZERO);
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    if (fesetround(round_original) != 0) {
        return 8;
    }
    cleared = fetestexcept(FE_INVALID | FE_INEXACT | FE_DIVBYZERO);
    if (fesetenv(&marked) != 0) {
        return 9;
    }
    restored = fetestexcept(FE_INVALID | FE_INEXACT | FE_DIVBYZERO);
    if (fesetenv(&original) != 0) {
        return 10;
    }
    round_restored = fegetround() == round_original ? 1 : 0;

    printf("fenv-env round=%d/%d invalid=%d inexact=%d divzero=%d cleared=%d restored=%d\n",
           round_changed,
           round_restored,
           (raised & FE_INVALID) ? 1 : 0,
           (raised & FE_INEXACT) ? 1 : 0,
           (raised & FE_DIVBYZERO) ? 1 : 0,
           cleared,
           ((restored & FE_INVALID) ? 1 : 0) + ((restored & FE_INEXACT) ? 2 : 0));

    return round_changed == 1 && round_restored == 1 && (raised & FE_INVALID) != 0 &&
                   (raised & FE_INEXACT) != 0 && (raised & FE_DIVBYZERO) == 0 &&
                   cleared == 0 && (restored & FE_INVALID) != 0 &&
                   (restored & FE_INEXACT) != 0 && (restored & FE_DIVBYZERO) == 0
               ? 0
               : 1;
}
