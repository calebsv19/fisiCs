#include <stdatomic.h>

static unsigned wave38_fetch_surface(atomic_uint *bits) {
    unsigned add_old = atomic_fetch_add_explicit(bits, 3U, memory_order_acq_rel);
    unsigned sub_old = atomic_fetch_sub_explicit(bits, 2U, memory_order_acq_rel);
    unsigned or_old = atomic_fetch_or_explicit(bits, 8U, memory_order_acq_rel);
    unsigned xor_old = atomic_fetch_xor_explicit(bits, 6U, memory_order_acq_rel);
    unsigned and_old = atomic_fetch_and_explicit(bits, 7U, memory_order_acq_rel);
    return add_old + sub_old + or_old + xor_old + and_old;
}

int main(void) {
    atomic_uint bits;
    atomic_init(&bits, 5U);
    return wave38_fetch_surface(&bits) == 41U ? 0 : 1;
}
