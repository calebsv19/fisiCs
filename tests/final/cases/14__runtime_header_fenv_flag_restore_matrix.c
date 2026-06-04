#include <fenv.h>
#include <stdio.h>

int main(void) {
    fexcept_t flags;
    int restored;

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }
    if (feraiseexcept(FE_INVALID | FE_OVERFLOW) != 0) {
        return 2;
    }
    if (fegetexceptflag(&flags, FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 3;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    if (fesetexceptflag(&flags, FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 5;
    }
    restored = fetestexcept(FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW);
    printf("fenv-flags invalid=%d overflow=%d underflow=%d\n",
           (restored & FE_INVALID) ? 1 : 0,
           (restored & FE_OVERFLOW) ? 1 : 0,
           (restored & FE_UNDERFLOW) ? 1 : 0);
    return 0;
}
