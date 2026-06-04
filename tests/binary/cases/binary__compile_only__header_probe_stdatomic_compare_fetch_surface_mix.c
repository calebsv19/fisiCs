#include <stdatomic.h>

static int header_probe_wave37_compare_fetch_surface(atomic_int *value, atomic_uint *mask) {
    int expected = 4;
    int swapped = atomic_compare_exchange_strong_explicit(
        value, &expected, 9, memory_order_acq_rel, memory_order_acquire);
    unsigned before_add = atomic_fetch_add_explicit(mask, 3U, memory_order_acq_rel);
    unsigned before_xor = atomic_fetch_xor_explicit(mask, 6U, memory_order_acq_rel);
    return swapped + expected + (int)before_add + (int)before_xor;
}

int main(void) {
    atomic_int value;
    atomic_uint mask;
    atomic_init(&value, 4);
    atomic_init(&mask, 5U);
    return header_probe_wave37_compare_fetch_surface(&value, &mask) == 0;
}
