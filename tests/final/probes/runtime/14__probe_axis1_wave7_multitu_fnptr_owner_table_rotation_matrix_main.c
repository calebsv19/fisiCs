#include <stdio.h>

extern unsigned axis1_wave7_fnptr_owner_fold(unsigned seed);

int main(void) {
    printf("%u %u\n", axis1_wave7_fnptr_owner_fold(9u), axis1_wave7_fnptr_owner_fold(14u));
    return 0;
}
