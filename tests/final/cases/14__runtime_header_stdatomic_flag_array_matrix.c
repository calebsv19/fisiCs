#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    atomic_flag flags[2] = { ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT };
    int first_a = atomic_flag_test_and_set_explicit(&flags[0], memory_order_acquire);
    int first_b = atomic_flag_test_and_set_explicit(&flags[1], memory_order_acquire);
    int second_a = atomic_flag_test_and_set_explicit(&flags[0], memory_order_acquire);
    atomic_flag_clear_explicit(&flags[0], memory_order_release);
    atomic_flag_clear_explicit(&flags[1], memory_order_release);

    printf("atomic-flag-array a=%d b=%d again=%d\n",
           first_a ? 1 : 0,
           first_b ? 1 : 0,
           second_a ? 1 : 0);
    return 0;
}
