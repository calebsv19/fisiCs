#include <stdio.h>

extern unsigned axis1_wave8_owner_hop_route_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave8_owner_hop_route_hash(37u);
    printf("%u %u\n", folded, folded % 9973u);
    return 0;
}
