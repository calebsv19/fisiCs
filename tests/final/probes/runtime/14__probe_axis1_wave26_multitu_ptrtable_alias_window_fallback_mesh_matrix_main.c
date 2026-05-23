#include <stdio.h>

extern unsigned axis1_wave26_ptrtable_alias_window_fallback_mesh_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave26_ptrtable_alias_window_fallback_mesh_matrix(241u),
           axis1_wave26_ptrtable_alias_window_fallback_mesh_matrix(373u));
    return 0;
}
