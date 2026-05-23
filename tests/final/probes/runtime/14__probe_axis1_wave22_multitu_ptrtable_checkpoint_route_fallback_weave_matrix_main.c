#include <stdio.h>

extern unsigned axis1_wave22_ptrtable_checkpoint_route_fallback_weave_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave22_ptrtable_checkpoint_route_fallback_weave_matrix(167u),
           axis1_wave22_ptrtable_checkpoint_route_fallback_weave_matrix(281u));
    return 0;
}
