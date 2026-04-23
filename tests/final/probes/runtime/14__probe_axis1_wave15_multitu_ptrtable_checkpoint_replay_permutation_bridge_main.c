#include <stdio.h>

extern unsigned axis1_wave15_ptrtable_checkpoint_replay_permute(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave15_ptrtable_checkpoint_replay_permute(41u),
           axis1_wave15_ptrtable_checkpoint_replay_permute(95u));
    return 0;
}
