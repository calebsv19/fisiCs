#include <stdio.h>

extern unsigned axis1_wave17_ptrtable_route_window_replay_alias_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave17_ptrtable_route_window_replay_alias_matrix(89u),
           axis1_wave17_ptrtable_route_window_replay_alias_matrix(167u));
    return 0;
}
