#include <stdatomic.h>

static int wave38_compare_exchange_surface(atomic_int *slot) {
    int expected = 3;
    int ok = atomic_compare_exchange_strong_explicit(
        slot, &expected, 11, memory_order_acq_rel, memory_order_acquire);
    int after = atomic_load_explicit(slot, memory_order_relaxed);
    return ok + expected + after;
}

int main(void) {
    atomic_int slot;
    atomic_init(&slot, 3);
    return wave38_compare_exchange_surface(&slot) == 15 ? 0 : 1;
}
