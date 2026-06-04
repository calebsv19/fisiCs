#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    atomic_uint bits;
    unsigned before_add = 0;
    unsigned before_sub = 0;
    unsigned before_or = 0;
    unsigned before_xor = 0;
    unsigned before_and = 0;
    unsigned final_bits = 0;

    atomic_init(&bits, 5U);
    before_add = atomic_fetch_add_explicit(&bits, 3U, memory_order_acq_rel);
    before_sub = atomic_fetch_sub_explicit(&bits, 2U, memory_order_acq_rel);
    before_or = atomic_fetch_or_explicit(&bits, 8U, memory_order_acq_rel);
    before_xor = atomic_fetch_xor_explicit(&bits, 6U, memory_order_acq_rel);
    before_and = atomic_fetch_and_explicit(&bits, 7U, memory_order_acq_rel);
    final_bits = atomic_load_explicit(&bits, memory_order_relaxed);

    if (before_add != 5U || before_sub != 8U || before_or != 6U ||
        before_xor != 14U || before_and != 8U || final_bits != 0U) {
        return 1;
    }

    printf("atomic-fetch add=%u sub=%u or=%u xor=%u and=%u final=%u\n",
           before_add,
           before_sub,
           before_or,
           before_xor,
           before_and,
           final_bits);
    return 0;
}
