#include <stdatomic.h>

int header_corpus_wave13_atomic_bridge_result(int seed);

int main(void) {
    return header_corpus_wave13_atomic_bridge_result(3) == 7 ? 0 : 1;
}
