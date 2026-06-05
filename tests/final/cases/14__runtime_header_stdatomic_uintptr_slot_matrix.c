#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    int cells[4] = {11, 22, 33, 44};
    atomic_uintptr_t slot;
    uintptr_t before;
    uintptr_t exchanged;
    uintptr_t after;
    long before_index;
    long exchanged_index;
    long after_index;
    int sum;

    atomic_init(&slot, (uintptr_t)&cells[1]);
    before = atomic_load_explicit(&slot, memory_order_relaxed);
    exchanged = atomic_exchange_explicit(&slot, (uintptr_t)&cells[3], memory_order_acq_rel);
    atomic_store_explicit(&slot, (uintptr_t)&cells[0], memory_order_release);
    after = atomic_load_explicit(&slot, memory_order_acquire);

    before_index = (long)(((int *)before) - cells);
    exchanged_index = (long)(((int *)exchanged) - cells);
    after_index = (long)(((int *)after) - cells);
    sum = ((int *)before)[0] + ((int *)exchanged)[0] + ((int *)after)[0] + cells[3];

    printf("atomic-uintptr before=%ld exchanged=%ld after=%ld sum=%d\n",
           before_index,
           exchanged_index,
           after_index,
           sum);

    return before_index == 1L && exchanged_index == 1L && after_index == 0L && sum == 99 ? 0 : 1;
}
