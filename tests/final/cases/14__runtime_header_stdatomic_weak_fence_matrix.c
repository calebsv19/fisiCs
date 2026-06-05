#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    atomic_int value;
    int expected = 3;
    int ok = 1;
    int final_value = 0;

    atomic_init(&value, 5);
    atomic_thread_fence(memory_order_seq_cst);
    ok = atomic_compare_exchange_weak_explicit(
        &value, &expected, 9, memory_order_acq_rel, memory_order_acquire);
    atomic_signal_fence(memory_order_acquire);
    final_value = atomic_load_explicit(&value, memory_order_relaxed);

    if (ok != 0 || expected != 5 || final_value != 5) {
        return 1;
    }

    printf("atomic-weak-fence ok=%d expected=%d value=%d\n",
           ok,
           expected,
           final_value);
    return 0;
}
