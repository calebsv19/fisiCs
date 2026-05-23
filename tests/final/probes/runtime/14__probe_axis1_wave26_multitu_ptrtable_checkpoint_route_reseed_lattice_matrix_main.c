#include <stdio.h>

extern unsigned axis1_wave26_ptrtable_checkpoint_route_reseed_lattice_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave26_ptrtable_checkpoint_route_reseed_lattice_matrix(239u),
           axis1_wave26_ptrtable_checkpoint_route_reseed_lattice_matrix(367u));
    return 0;
}
