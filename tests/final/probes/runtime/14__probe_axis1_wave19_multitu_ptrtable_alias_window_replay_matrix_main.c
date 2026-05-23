#include <stdio.h>

extern unsigned axis1_wave19_ptrtable_alias_window_replay_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave19_ptrtable_alias_window_replay_matrix(113u),
           axis1_wave19_ptrtable_alias_window_replay_matrix(227u));
    return 0;
}
