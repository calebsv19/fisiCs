#include <stdio.h>

extern unsigned axis1_wave10_fnptr_owner_checkpoint_fold(unsigned seed);

int main(void) {
    printf("%u %u\n", axis1_wave10_fnptr_owner_checkpoint_fold(7u), axis1_wave10_fnptr_owner_checkpoint_fold(22u));
    return 0;
}
