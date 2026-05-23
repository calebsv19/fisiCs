#include <stdio.h>

extern unsigned axis1_wave23_ptrtable_alias_window_fallback_reseed_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave23_ptrtable_alias_window_fallback_reseed_matrix(191u),
           axis1_wave23_ptrtable_alias_window_fallback_reseed_matrix(313u));
    return 0;
}
