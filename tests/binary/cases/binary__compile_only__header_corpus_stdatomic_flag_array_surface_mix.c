#include <stdatomic.h>

static int header_corpus_wave37_flag_array_surface(atomic_flag flags[2]) {
    int first = atomic_flag_test_and_set_explicit(&flags[0], memory_order_acquire);
    int second = atomic_flag_test_and_set_explicit(&flags[1], memory_order_acquire);
    atomic_flag_clear_explicit(&flags[0], memory_order_release);
    atomic_flag_clear_explicit(&flags[1], memory_order_release);
    return first + second;
}

int main(void) {
    atomic_flag flags[2] = { ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT };
    return header_corpus_wave37_flag_array_surface(flags);
}
