#include <stdio.h>

extern unsigned axis1_wave22_ptrtable_alias_replay_braid_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave22_ptrtable_alias_replay_braid_matrix(173u),
           axis1_wave22_ptrtable_alias_replay_braid_matrix(293u));
    return 0;
}
