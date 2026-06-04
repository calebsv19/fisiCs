#include <fenv.h>
#include <stdio.h>

int main(void) {
    fenv_t saved;
    int held;
    int after;

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }
    if (feraiseexcept(FE_DIVBYZERO) != 0) {
        return 2;
    }
    if (feholdexcept(&saved) != 0) {
        return 3;
    }
    held = fetestexcept(FE_DIVBYZERO | FE_INEXACT);
    if (feraiseexcept(FE_INEXACT) != 0) {
        return 4;
    }
    if (feupdateenv(&saved) != 0) {
        return 5;
    }
    after = fetestexcept(FE_DIVBYZERO | FE_INEXACT | FE_INVALID);
    printf("fenv-hold held=%d div=%d inexact=%d invalid=%d\n",
           held == 0 ? 1 : 0,
           (after & FE_DIVBYZERO) ? 1 : 0,
           (after & FE_INEXACT) ? 1 : 0,
           (after & FE_INVALID) ? 1 : 0);
    return 0;
}
