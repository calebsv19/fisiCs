#include <stdio.h>

extern unsigned axis1_wave31_fnptr_owner_route_shadow_checkpoint_matrix(unsigned seed);

int main(void) {
    printf("%u %u\n",
           axis1_wave31_fnptr_owner_route_shadow_checkpoint_matrix(63u),
           axis1_wave31_fnptr_owner_route_shadow_checkpoint_matrix(118u));
    return 0;
}
