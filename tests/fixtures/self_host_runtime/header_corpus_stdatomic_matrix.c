#include <stdatomic.h>
#include <stdio.h>

static int header_corpus_wave13_stdatomic_summary(void) {
    _Atomic int counter;
    _Atomic unsigned long state;
    int loaded = 0;
    int old = 0;
    unsigned long loaded_ul = 0UL;
    unsigned long old_ul = 0UL;

    atomic_init(&counter, 4);
    atomic_init(&state, 6UL);

    loaded = atomic_load_explicit(&counter, memory_order_acquire);
    atomic_store_explicit(&counter, loaded + 3, memory_order_release);
    old = atomic_exchange_explicit(&counter, 11, memory_order_acq_rel);

    loaded_ul = atomic_load_explicit(&state, memory_order_relaxed);
    atomic_store_explicit(&state, loaded_ul + 5UL, memory_order_release);
    old_ul = atomic_exchange_explicit(&state, 21UL, memory_order_acq_rel);

    return loaded * 1000 + old * 100 + (int)loaded_ul * 10 + (int)old_ul;
}

int main(void) {
    int summary = header_corpus_wave13_stdatomic_summary();

    if (summary != 4761) {
        return 1;
    }

    printf("summary=%d\n", summary);
    return 0;
}
