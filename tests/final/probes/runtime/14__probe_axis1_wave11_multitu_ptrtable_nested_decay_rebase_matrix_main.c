#include <stdio.h>

extern unsigned axis1_wave11_ptrtable_nested_decay_fold(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave11_ptrtable_nested_decay_fold(19u),
           axis1_wave11_ptrtable_nested_decay_fold(44u));
    return 0;
}
