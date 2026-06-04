#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    atomic_int value;
    int expected = 4;
    int ok1 = 0;
    int ok2 = 0;
    int final_value = 0;

    atomic_init(&value, 4);
    ok1 = atomic_compare_exchange_strong_explicit(
        &value, &expected, 9, memory_order_acq_rel, memory_order_acquire);
    expected = 7;
    ok2 = atomic_compare_exchange_strong_explicit(
        &value, &expected, 12, memory_order_acq_rel, memory_order_acquire);
    final_value = atomic_load_explicit(&value, memory_order_relaxed);

    if (ok1 != 1 || ok2 != 0 || expected != 9 || final_value != 9) {
        return 1;
    }

    printf("atomic-compare ok1=%d ok2=%d value=%d expected=%d\n",
           ok1,
           ok2,
           final_value,
           expected);
    return 0;
}
