#include <stdio.h>

extern unsigned axis1_wave28_ptrtable_checkpoint_route_fallback_arc_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave28_ptrtable_checkpoint_route_fallback_arc_matrix(271u),
           axis1_wave28_ptrtable_checkpoint_route_fallback_arc_matrix(397u));
    return 0;
}
