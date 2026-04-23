#include <stdio.h>

extern unsigned axis1_wave13_ptrtable_constexpr_offset_blend(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave13_ptrtable_constexpr_offset_blend(31u),
           axis1_wave13_ptrtable_constexpr_offset_blend(63u));
    return 0;
}
