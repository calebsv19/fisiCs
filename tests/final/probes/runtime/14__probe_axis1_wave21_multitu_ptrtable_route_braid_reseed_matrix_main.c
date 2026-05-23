#include <stdio.h>

extern unsigned axis1_wave21_ptrtable_route_braid_reseed_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave21_ptrtable_route_braid_reseed_matrix(157u),
           axis1_wave21_ptrtable_route_braid_reseed_matrix(271u));
    return 0;
}
