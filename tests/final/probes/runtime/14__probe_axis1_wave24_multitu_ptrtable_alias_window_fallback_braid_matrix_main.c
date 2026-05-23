#include <stdio.h>

extern unsigned axis1_wave24_ptrtable_alias_window_fallback_braid_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave24_ptrtable_alias_window_fallback_braid_matrix(211u),
           axis1_wave24_ptrtable_alias_window_fallback_braid_matrix(337u));
    return 0;
}
