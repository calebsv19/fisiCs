#include <stdio.h>

extern unsigned axis1_wave6_owner_hop_reduced_fold(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave6_owner_hop_reduced_fold(7u);
    printf("%u\n", folded);
    return 0;
}
