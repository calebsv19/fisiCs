#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    _Atomic int counter;
    int loaded = 0;
    int old = 0;
    int final = 0;

    atomic_init(&counter, 1);
    loaded = atomic_load_explicit(&counter, memory_order_acquire);
    atomic_store_explicit(&counter, loaded + 1, memory_order_release);
    old = atomic_exchange_explicit(&counter, 7, memory_order_acq_rel);
    final = atomic_load_explicit(&counter, memory_order_relaxed);

    if (loaded != 1) {
        return 1;
    }
    if (old != 2) {
        return 2;
    }
    if (final != 7) {
        return 3;
    }

    printf("loaded=%d old=%d final=%d\n", loaded, old, final);
    return 0;
}
