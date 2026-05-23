#include <stdio.h>

extern unsigned axis1_wave16_ptrtable_epoch_window_rewind_bridge(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave16_ptrtable_epoch_window_rewind_bridge(47u),
           axis1_wave16_ptrtable_epoch_window_rewind_bridge(103u));
    return 0;
}
