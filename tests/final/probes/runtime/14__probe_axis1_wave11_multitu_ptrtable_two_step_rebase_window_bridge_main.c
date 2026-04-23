#include <stdio.h>

extern unsigned axis1_wave11_ptrtable_two_step_rebase_bridge(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave11_ptrtable_two_step_rebase_bridge(13u),
           axis1_wave11_ptrtable_two_step_rebase_bridge(37u));
    return 0;
}
