#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    atomic_flag flag = ATOMIC_FLAG_INIT;
    int first = atomic_flag_test_and_set_explicit(&flag, memory_order_acquire);
    int second = atomic_flag_test_and_set_explicit(&flag, memory_order_acquire);
    atomic_flag_clear_explicit(&flag, memory_order_release);
    int third = atomic_flag_test_and_set_explicit(&flag, memory_order_acquire);

    printf("atomic-flag first=%d second=%d third=%d\n",
           first ? 1 : 0,
           second ? 1 : 0,
           third ? 1 : 0);
    return 0;
}
