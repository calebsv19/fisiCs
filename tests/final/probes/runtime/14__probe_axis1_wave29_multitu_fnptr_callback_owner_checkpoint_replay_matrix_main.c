#include <stdio.h>

extern unsigned axis1_wave29_fnptr_callback_owner_checkpoint_replay_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave29_fnptr_callback_owner_checkpoint_replay_matrix(41u),
           axis1_wave29_fnptr_callback_owner_checkpoint_replay_matrix(92u));
    return 0;
}
