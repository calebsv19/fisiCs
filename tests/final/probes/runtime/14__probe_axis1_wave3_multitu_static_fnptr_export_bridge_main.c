#include <stdio.h>

extern int axis1_wave3_apply(int seed);

int main(void) {
    printf("%d\n", axis1_wave3_apply(12));
    return 0;
}
