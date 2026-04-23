#include <stdio.h>

extern unsigned axis1_wave7_owner_hop_depth_hash_current(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave7_owner_hop_depth_hash_current(13u);
    printf("%u\n", folded);
    return 0;
}
