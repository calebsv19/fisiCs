#include <stdatomic.h>
#include <stdio.h>

int main(void) {
    atomic_bool gate;
    atomic_schar delta;
    int old_gate;
    int gate_after;
    signed char old_delta;
    signed char delta_after;
    int summary;

    atomic_init(&gate, 0);
    atomic_init(&delta, (signed char)-3);

    old_gate = atomic_exchange_explicit(&gate, 1, memory_order_acq_rel) ? 1 : 0;
    gate_after = atomic_load_explicit(&gate, memory_order_acquire) ? 1 : 0;
    atomic_store_explicit(&delta, (signed char)7, memory_order_release);
    old_delta = atomic_exchange_explicit(&delta, (signed char)-9, memory_order_acq_rel);
    delta_after = atomic_load_explicit(&delta, memory_order_acquire);
    summary = old_gate + gate_after * 3 + (int)old_delta * 5 + (int)delta_after * 7;

    printf("atomic-bool-char old_gate=%d gate=%d old_delta=%d delta=%d summary=%d\n",
           old_gate,
           gate_after,
           (int)old_delta,
           (int)delta_after,
           summary);

    return old_gate == 0 && gate_after == 1 && old_delta == 7 && delta_after == -9 &&
                   summary == -25
               ? 0
               : 1;
}
