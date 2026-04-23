#include <stdio.h>

extern unsigned axis1_wave15_ptrtable_tu_order_flip_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave15_ptrtable_tu_order_flip_hash(101u);
    printf("%u %u\n", folded, folded % 17497u);
    return 0;
}
