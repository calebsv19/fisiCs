#include <stdio.h>

extern unsigned axis1_wave12_ptrtable_signed_unsigned_bridge(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave12_ptrtable_signed_unsigned_bridge(17u),
           axis1_wave12_ptrtable_signed_unsigned_bridge(61u));
    return 0;
}
