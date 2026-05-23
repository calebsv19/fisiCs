#include <stdio.h>

extern unsigned axis1_wave27_ptrtable_alias_window_reseed_arc_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave27_ptrtable_alias_window_reseed_arc_matrix(263u),
           axis1_wave27_ptrtable_alias_window_reseed_arc_matrix(389u));
    return 0;
}
