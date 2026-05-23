#include <stdio.h>

extern unsigned axis1_wave23_ptrtable_checkpoint_route_braid_collapse_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave23_ptrtable_checkpoint_route_braid_collapse_matrix(181u),
           axis1_wave23_ptrtable_checkpoint_route_braid_collapse_matrix(307u));
    return 0;
}
