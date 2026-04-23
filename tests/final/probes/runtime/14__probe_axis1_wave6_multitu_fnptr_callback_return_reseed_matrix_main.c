#include <stdio.h>

extern int axis1_wave6_callback_eval(int seed);

int main(void) {
    printf("%d %d\n", axis1_wave6_callback_eval(6), axis1_wave6_callback_eval(11));
    return 0;
}
