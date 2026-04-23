#include <stdio.h>

extern int axis1_wave3_part_a(int value);
extern int axis1_wave3_part_b(int value);
extern const int axis1_wave3_bias;

int main(void) {
    int value = axis1_wave3_part_b(axis1_wave3_part_a(6)) + axis1_wave3_bias;
    printf("%d %d\n", value, axis1_wave3_bias);
    return 0;
}
