#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    _Atomic unsigned long state;
    unsigned long loaded = 0UL;
    unsigned long old = 0UL;
    unsigned long final = 0UL;

    atomic_init(&state, 3UL);
    loaded = atomic_load_explicit(&state, memory_order_relaxed);
    atomic_store_explicit(&state, loaded + 4UL, memory_order_release);
    old = atomic_exchange_explicit(&state, 12UL, memory_order_acq_rel);
    final = atomic_load_explicit(&state, memory_order_acquire);

    if (loaded != 3UL) {
        return 1;
    }
    if (old != 7UL) {
        return 2;
    }
    if (final != 12UL) {
        return 3;
    }

    printf("loaded=%lu old=%lu final=%lu\n", loaded, old, final);
    return 0;
}
