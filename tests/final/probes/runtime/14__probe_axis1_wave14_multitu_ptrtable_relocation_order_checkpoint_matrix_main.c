#include <stdio.h>

extern unsigned axis1_wave14_ptrtable_relocation_order_fold(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave14_ptrtable_relocation_order_fold(43u),
           axis1_wave14_ptrtable_relocation_order_fold(79u));
    return 0;
}
