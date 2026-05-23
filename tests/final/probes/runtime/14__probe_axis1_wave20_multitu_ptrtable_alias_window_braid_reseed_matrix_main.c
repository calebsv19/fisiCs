#include <stdio.h>

extern unsigned axis1_wave20_ptrtable_alias_window_braid_reseed_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave20_ptrtable_alias_window_braid_reseed_matrix(139u),
           axis1_wave20_ptrtable_alias_window_braid_reseed_matrix(251u));
    return 0;
}
