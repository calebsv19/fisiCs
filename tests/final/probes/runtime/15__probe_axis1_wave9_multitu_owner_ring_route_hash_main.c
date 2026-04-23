#include <stdio.h>

extern unsigned axis1_wave9_owner_ring_route_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave9_owner_ring_route_hash(29u);
    printf("%u %u\n", folded, folded % 7919u);
    return 0;
}
