#include <stdio.h>

extern unsigned axis1_wave19_ptrtable_checkpoint_braid_fallback_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave19_ptrtable_checkpoint_braid_fallback_matrix(101u),
           axis1_wave19_ptrtable_checkpoint_braid_fallback_matrix(211u));
    return 0;
}
