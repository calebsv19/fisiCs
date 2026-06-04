#include <stdatomic.h>

int wave38_compare_bridge(atomic_int *slot);
unsigned wave38_fetch_bridge(atomic_uint *mask);

int main(void) {
    atomic_int slot;
    atomic_uint mask;
    atomic_init(&slot, 2);
    atomic_init(&mask, 5U);
    return wave38_compare_bridge(&slot) == 7 &&
                   wave38_fetch_bridge(&mask) == 32U
               ? 0
               : 1;
}
