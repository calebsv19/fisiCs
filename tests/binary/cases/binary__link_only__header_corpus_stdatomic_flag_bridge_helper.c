#include <stdatomic.h>

int header_corpus_wave37_atomic_flag_score(atomic_flag *flag) {
    int first = atomic_flag_test_and_set_explicit(flag, memory_order_acquire);
    int second = atomic_flag_test_and_set_explicit(flag, memory_order_acquire);
    atomic_flag_clear_explicit(flag, memory_order_release);
    int third = atomic_flag_test_and_set_explicit(flag, memory_order_acquire);
    return first + (second ? 2 : 0) + third;
}
