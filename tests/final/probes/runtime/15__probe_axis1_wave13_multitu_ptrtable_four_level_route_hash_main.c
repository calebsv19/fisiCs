#include <stdio.h>

extern unsigned axis1_wave13_ptrtable_four_level_route_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave13_ptrtable_four_level_route_hash(67u);
    printf("%u %u\n", folded, folded % 12289u);
    return 0;
}
