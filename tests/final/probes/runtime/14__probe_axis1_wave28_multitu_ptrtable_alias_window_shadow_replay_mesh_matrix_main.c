#include <stdio.h>

extern unsigned axis1_wave28_ptrtable_alias_window_shadow_replay_mesh_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave28_ptrtable_alias_window_shadow_replay_mesh_matrix(283u),
           axis1_wave28_ptrtable_alias_window_shadow_replay_mesh_matrix(409u));
    return 0;
}
