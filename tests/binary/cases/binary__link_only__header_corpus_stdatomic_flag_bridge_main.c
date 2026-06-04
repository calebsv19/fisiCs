#include <stdatomic.h>

int header_corpus_wave37_atomic_flag_score(atomic_flag *flag);

int main(void) {
    atomic_flag flag = ATOMIC_FLAG_INIT;
    return header_corpus_wave37_atomic_flag_score(&flag) == 2 ? 0 : 1;
}
