#include <stdatomic.h>

static int header_corpus_wave37_flag_surface(atomic_flag *flag) {
    int first = atomic_flag_test_and_set_explicit(flag, memory_order_acquire);
    atomic_flag_clear_explicit(flag, memory_order_release);
    return first;
}

int main(void) {
    atomic_flag flag = ATOMIC_FLAG_INIT;
    return header_corpus_wave37_flag_surface(&flag);
}
