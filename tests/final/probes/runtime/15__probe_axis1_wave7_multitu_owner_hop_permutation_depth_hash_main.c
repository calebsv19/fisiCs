#include <stdio.h>

extern unsigned axis1_wave7_owner_hop_depth_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave7_owner_hop_depth_hash(31u);
    printf("%u %u\n", folded, folded % 1021u);
    return 0;
}
