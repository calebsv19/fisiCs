#include <stdio.h>

extern unsigned axis1_wave5_owner_fold(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave5_owner_fold(11u);
    printf("%u %u\n", folded, folded ^ 0x5Au);
    return 0;
}
