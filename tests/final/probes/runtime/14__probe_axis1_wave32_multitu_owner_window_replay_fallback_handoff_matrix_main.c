#include <stdio.h>

extern unsigned axis1_wave32_owner_window_replay_fallback_handoff_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave32_owner_window_replay_fallback_handoff_matrix(71u),
           axis1_wave32_owner_window_replay_fallback_handoff_matrix(133u));
    return 0;
}
