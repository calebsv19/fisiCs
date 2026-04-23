#include <stdio.h>

extern int axis1_wave4_apply(int seed);

int main(void) {
    printf("%d %d\n", axis1_wave4_apply(3), axis1_wave4_apply(8));
    return 0;
}
