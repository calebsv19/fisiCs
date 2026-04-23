#include <stdio.h>

extern unsigned axis1_wave7_owner_depth_fold(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave7_owner_depth_fold(19u);
    printf("%u %u\n", folded, folded % 997u);
    return 0;
}
