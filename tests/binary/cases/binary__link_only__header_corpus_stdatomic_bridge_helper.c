#include <stdatomic.h>

int header_corpus_wave13_atomic_bridge_result(int seed) {
    _Atomic int counter;
    int loaded = 0;
    int previous = 0;

    atomic_init(&counter, seed);
    loaded = atomic_load_explicit(&counter, memory_order_acquire);
    atomic_store_explicit(&counter, loaded + 1, memory_order_release);
    previous = atomic_exchange_explicit(&counter, loaded + 4, memory_order_acq_rel);
    return previous + atomic_load_explicit(&counter, memory_order_relaxed);
}
