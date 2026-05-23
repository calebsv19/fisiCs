#include <stdio.h>

extern unsigned axis1_wave24_ptrtable_checkpoint_route_replay_collapse_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave24_ptrtable_checkpoint_route_replay_collapse_matrix(197u),
           axis1_wave24_ptrtable_checkpoint_route_replay_collapse_matrix(331u));
    return 0;
}
