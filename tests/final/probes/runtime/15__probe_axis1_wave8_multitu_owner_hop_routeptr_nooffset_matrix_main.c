#include <stdio.h>

extern unsigned axis1_wave8_owner_hop_nooffset_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave8_owner_hop_nooffset_hash(17u);
    printf("%u\n", folded);
    return 0;
}
