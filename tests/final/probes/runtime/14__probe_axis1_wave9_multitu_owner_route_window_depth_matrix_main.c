#include <stdio.h>

extern unsigned axis1_wave9_owner_route_window_depth(unsigned seed);

int main(void) {
    unsigned folded = axis1_wave9_owner_route_window_depth(19u);
    printf("%u %u\n", folded, folded % 10007u);
    return 0;
}
