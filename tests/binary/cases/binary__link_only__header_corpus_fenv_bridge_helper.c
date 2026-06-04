#include <fenv.h>

int header_corpus_wave12_rounding_score(int round_mode) {
    switch (round_mode) {
        case FE_TONEAREST:
            return 0;
        case FE_UPWARD:
        case FE_DOWNWARD:
        case FE_TOWARDZERO:
            return 1;
        default:
            return -1;
    }
}
