#include <stdio.h>

extern int axis1_wave6_permute_eval(int seed);

int main(void) {
    printf("%d %d\n", axis1_wave6_permute_eval(5), axis1_wave6_permute_eval(12));
    return 0;
}
