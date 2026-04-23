#include <stdio.h>

extern unsigned axis1_wave15_ptrtable_three_plan_rebase_fold(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave15_ptrtable_three_plan_rebase_fold(59u),
           axis1_wave15_ptrtable_three_plan_rebase_fold(83u));
    return 0;
}
