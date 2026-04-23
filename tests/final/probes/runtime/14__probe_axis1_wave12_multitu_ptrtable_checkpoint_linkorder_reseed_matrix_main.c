#include <stdio.h>

extern unsigned axis1_wave12_ptrtable_checkpoint_linkorder_fold(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave12_ptrtable_checkpoint_linkorder_fold(23u),
           axis1_wave12_ptrtable_checkpoint_linkorder_fold(52u));
    return 0;
}
