#include <stdio.h>

extern unsigned axis1_wave10_owner_checkpoint_route_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave10_owner_checkpoint_route_hash(41u);
    printf("%u %u\n", folded, folded % 6553u);
    return 0;
}
