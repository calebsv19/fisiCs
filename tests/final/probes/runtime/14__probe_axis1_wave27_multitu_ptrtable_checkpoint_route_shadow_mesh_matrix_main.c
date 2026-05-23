#include <stdio.h>

extern unsigned axis1_wave27_ptrtable_checkpoint_route_shadow_mesh_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave27_ptrtable_checkpoint_route_shadow_mesh_matrix(251u),
           axis1_wave27_ptrtable_checkpoint_route_shadow_mesh_matrix(379u));
    return 0;
}
