#include <stdio.h>

extern unsigned axis1_wave10_owner_checkpoint_window_fold(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave10_owner_checkpoint_window_fold(33u);
    printf("%u %u\n", folded, folded % 8191u);
    return 0;
}
