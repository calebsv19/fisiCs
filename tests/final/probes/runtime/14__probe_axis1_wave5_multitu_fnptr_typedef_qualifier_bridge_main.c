#include <stdio.h>

extern int axis1_wave5_bridge_eval(int seed);

int main(void) {
    printf("%d %d\n", axis1_wave5_bridge_eval(4), axis1_wave5_bridge_eval(9));
    return 0;
}
