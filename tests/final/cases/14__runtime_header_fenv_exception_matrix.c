#include <fenv.h>
#include <stdio.h>

int main(void) {
    int mask = 0;

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }
    if (feraiseexcept(FE_DIVBYZERO | FE_INEXACT) != 0) {
        return 2;
    }

    mask = fetestexcept(FE_DIVBYZERO | FE_INEXACT | FE_INVALID);
    if ((mask & FE_DIVBYZERO) == 0) {
        return 3;
    }
    if ((mask & FE_INEXACT) == 0) {
        return 4;
    }
    if ((mask & FE_INVALID) != 0) {
        return 5;
    }

    printf(
        "divzero=%d inexact=%d invalid=%d\n",
        (mask & FE_DIVBYZERO) != 0,
        (mask & FE_INEXACT) != 0,
        (mask & FE_INVALID) != 0);
    return 0;
}
