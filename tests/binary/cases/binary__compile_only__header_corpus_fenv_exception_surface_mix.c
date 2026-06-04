#include <fenv.h>

int header_corpus_wave12_capture_excepts(double x) {
    fexcept_t saved;
    int raised = fetestexcept(FE_ALL_EXCEPT);

    if (fegetexceptflag(&saved, FE_DIVBYZERO | FE_INVALID) != 0) {
        return raised;
    }

    if (x < 0.0) {
        feraiseexcept(FE_INVALID);
    } else {
        feclearexcept(FE_ALL_EXCEPT);
    }

    fesetexceptflag(&saved, FE_DIVBYZERO | FE_INVALID);
    return raised;
}
