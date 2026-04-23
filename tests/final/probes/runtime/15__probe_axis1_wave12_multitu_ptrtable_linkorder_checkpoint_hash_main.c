#include <stdio.h>

extern unsigned axis1_wave12_ptrtable_linkorder_checkpoint_hash(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave12_ptrtable_linkorder_checkpoint_hash(53u);
    printf("%u %u\n", folded, folded % 9973u);
    return 0;
}
