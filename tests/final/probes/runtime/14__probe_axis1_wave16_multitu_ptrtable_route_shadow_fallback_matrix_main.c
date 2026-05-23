#include <stdio.h>

extern unsigned axis1_wave16_ptrtable_route_shadow_fallback_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave16_ptrtable_route_shadow_fallback_matrix(61u),
           axis1_wave16_ptrtable_route_shadow_fallback_matrix(127u));
    return 0;
}
