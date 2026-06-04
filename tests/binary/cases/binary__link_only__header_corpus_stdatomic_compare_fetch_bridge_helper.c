#include <stdatomic.h>

int wave38_compare_bridge(atomic_int *slot) {
    int expected = 2;
    int ok = atomic_compare_exchange_strong_explicit(
        slot, &expected, 7, memory_order_acq_rel, memory_order_acquire);
    int now = atomic_load_explicit(slot, memory_order_relaxed);
    return ok ? now : expected;
}

unsigned wave38_fetch_bridge(atomic_uint *mask) {
    unsigned before_or = atomic_fetch_or_explicit(mask, 8U, memory_order_acq_rel);
    unsigned before_xor = atomic_fetch_xor_explicit(mask, 6U, memory_order_acq_rel);
    unsigned before_and = atomic_fetch_and_explicit(mask, 7U, memory_order_acq_rel);
    unsigned now = atomic_load_explicit(mask, memory_order_relaxed);
    return before_or + before_xor + before_and + now;
}
