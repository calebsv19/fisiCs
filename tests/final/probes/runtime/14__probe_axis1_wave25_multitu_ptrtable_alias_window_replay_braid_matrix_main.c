#include <stdio.h>

extern unsigned axis1_wave25_ptrtable_alias_window_replay_braid_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave25_ptrtable_alias_window_replay_braid_matrix(227u),
           axis1_wave25_ptrtable_alias_window_replay_braid_matrix(353u));
    return 0;
}
