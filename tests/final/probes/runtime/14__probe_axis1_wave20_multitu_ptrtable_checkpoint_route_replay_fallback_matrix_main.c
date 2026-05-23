#include <stdio.h>

extern unsigned axis1_wave20_ptrtable_checkpoint_route_replay_fallback_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave20_ptrtable_checkpoint_route_replay_fallback_matrix(127u),
           axis1_wave20_ptrtable_checkpoint_route_replay_fallback_matrix(241u));
    return 0;
}
