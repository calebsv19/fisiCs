#include <stdio.h>

extern unsigned axis1_wave11_ptrtable_nested_decay_route_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave11_ptrtable_nested_decay_route_hash(47u);
    printf("%u %u\n", folded, folded % 8191u);
    return 0;
}
