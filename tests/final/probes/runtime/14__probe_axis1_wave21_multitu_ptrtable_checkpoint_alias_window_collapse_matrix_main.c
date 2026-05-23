#include <stdio.h>

extern unsigned axis1_wave21_ptrtable_checkpoint_alias_window_collapse_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave21_ptrtable_checkpoint_alias_window_collapse_matrix(149u),
           axis1_wave21_ptrtable_checkpoint_alias_window_collapse_matrix(263u));
    return 0;
}
