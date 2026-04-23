#include <stdio.h>

extern unsigned axis1_wave9_fnptr_owner_ring_fold(unsigned seed);

int main(void) {
    printf("%u %u\n", axis1_wave9_fnptr_owner_ring_fold(11u), axis1_wave9_fnptr_owner_ring_fold(26u));
    return 0;
}
