#include <stdio.h>

extern unsigned axis1_wave6_owner_hop_fold(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave6_owner_hop_fold(23u);
    printf("%u %u\n", folded, folded % 257u);
    return 0;
}
