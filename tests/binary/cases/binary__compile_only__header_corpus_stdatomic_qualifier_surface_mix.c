#include <stdatomic.h>

typedef _Atomic(unsigned long) atomic_ulong_local;

struct header_corpus_wave13_atomic_pair {
    atomic_int head;
    atomic_ulong_local tail;
};

int header_corpus_wave13_atomic_seed(struct header_corpus_wave13_atomic_pair *pair) {
    if (!pair) {
        return 0;
    }

    atomic_init(&pair->head, 1);
    atomic_init(&pair->tail, 2UL);
    return 1;
}
