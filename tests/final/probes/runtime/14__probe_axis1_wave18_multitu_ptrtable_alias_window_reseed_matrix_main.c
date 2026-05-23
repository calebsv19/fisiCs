#include <stdio.h>

extern unsigned axis1_wave18_ptrtable_alias_window_reseed_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave18_ptrtable_alias_window_reseed_matrix(109u),
           axis1_wave18_ptrtable_alias_window_reseed_matrix(193u));
    return 0;
}
