#include <fenv.h>

int header_corpus_wave12_rounding_score(int round_mode);

int main(void) {
    int round_mode = fegetround();
    return header_corpus_wave12_rounding_score(round_mode) >= 0 ? 0 : 1;
}
