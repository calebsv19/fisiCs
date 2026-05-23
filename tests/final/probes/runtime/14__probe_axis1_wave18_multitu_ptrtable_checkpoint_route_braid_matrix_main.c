#include <stdio.h>

extern unsigned axis1_wave18_ptrtable_checkpoint_route_braid_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave18_ptrtable_checkpoint_route_braid_matrix(97u),
           axis1_wave18_ptrtable_checkpoint_route_braid_matrix(181u));
    return 0;
}
