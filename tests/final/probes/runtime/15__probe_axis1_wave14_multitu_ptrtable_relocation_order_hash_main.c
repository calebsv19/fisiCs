#include <stdio.h>

extern unsigned axis1_wave14_ptrtable_relocation_order_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave14_ptrtable_relocation_order_hash(71u);
    printf("%u %u\n", folded, folded % 16381u);
    return 0;
}
