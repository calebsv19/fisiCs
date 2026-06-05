#include <fenv.h>
#include <stdio.h>

int main(void) {
    fexcept_t first_flags;
    fexcept_t second_flags;
    int first;
    int second;
    int combined;

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }
    if (feraiseexcept(FE_OVERFLOW) != 0) {
        return 2;
    }
    if (fegetexceptflag(&first_flags, FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT) != 0) {
        return 3;
    }
    first = fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT);
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    if (feraiseexcept(FE_UNDERFLOW | FE_INEXACT) != 0) {
        return 5;
    }
    if (fegetexceptflag(&second_flags, FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT) != 0) {
        return 6;
    }
    second = fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT);
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    if (fesetexceptflag(&first_flags, FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT) != 0) {
        return 8;
    }
    if (fesetexceptflag(&second_flags, FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT) != 0) {
        return 9;
    }
    combined = fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT);
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }

    printf("fenv-seq first=%d/%d/%d second=%d/%d/%d combined=%d/%d/%d\n",
           (first & FE_OVERFLOW) ? 1 : 0,
           (first & FE_UNDERFLOW) ? 1 : 0,
           (first & FE_INEXACT) ? 1 : 0,
           (second & FE_OVERFLOW) ? 1 : 0,
           (second & FE_UNDERFLOW) ? 1 : 0,
           (second & FE_INEXACT) ? 1 : 0,
           (combined & FE_OVERFLOW) ? 1 : 0,
           (combined & FE_UNDERFLOW) ? 1 : 0,
           (combined & FE_INEXACT) ? 1 : 0);

    return (first & FE_OVERFLOW) != 0 && (first & FE_UNDERFLOW) == 0 &&
                   (first & FE_INEXACT) != 0 &&
                   (second & FE_OVERFLOW) == 0 && (second & FE_UNDERFLOW) != 0 &&
                   (second & FE_INEXACT) != 0 && (combined & FE_OVERFLOW) == 0 &&
                   (combined & FE_UNDERFLOW) != 0 && (combined & FE_INEXACT) != 0
               ? 0
               : 1;
}
