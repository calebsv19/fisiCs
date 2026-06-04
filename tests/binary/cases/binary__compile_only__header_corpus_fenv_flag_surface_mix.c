#include <fenv.h>

static int header_corpus_wave36_flag_surface(int raise_invalid) {
    fexcept_t flags;
    int mask = FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW;
    feclearexcept(FE_ALL_EXCEPT);
    if (raise_invalid) {
        feraiseexcept(FE_INVALID);
    }
    fegetexceptflag(&flags, mask);
    feclearexcept(mask);
    fesetexceptflag(&flags, mask);
    return fetestexcept(mask);
}

int main(void) {
    return header_corpus_wave36_flag_surface(1) == 0;
}
