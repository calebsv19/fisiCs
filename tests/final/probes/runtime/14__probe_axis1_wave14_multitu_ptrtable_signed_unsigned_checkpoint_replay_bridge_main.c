#include <stdio.h>

extern unsigned axis1_wave14_ptrtable_checkpoint_replay_bridge(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave14_ptrtable_checkpoint_replay_bridge(27u),
           axis1_wave14_ptrtable_checkpoint_replay_bridge(73u));
    return 0;
}
