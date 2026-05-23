#include <stdio.h>

extern unsigned axis1_wave25_ptrtable_checkpoint_route_fallback_mesh_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave25_ptrtable_checkpoint_route_fallback_mesh_matrix(223u),
           axis1_wave25_ptrtable_checkpoint_route_fallback_mesh_matrix(349u));
    return 0;
}
