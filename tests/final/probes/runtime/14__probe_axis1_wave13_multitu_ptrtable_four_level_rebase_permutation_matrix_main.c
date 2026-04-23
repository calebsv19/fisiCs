#include <stdio.h>

extern unsigned axis1_wave13_ptrtable_four_level_rebase_fold(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave13_ptrtable_four_level_rebase_fold(29u),
           axis1_wave13_ptrtable_four_level_rebase_fold(58u));
    return 0;
}
