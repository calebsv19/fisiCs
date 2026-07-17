#include <stdio.h>

enum Mode {
    MODE_LOW = -3,
    MODE_MID = 5,
    MODE_HIGH = 12
};

int main(void) {
    signed char negative = -5;
    unsigned char wide = 250u;
    unsigned short narrow_sum = (unsigned short)(wide + (unsigned char)10u);
    int enum_promoted = (MODE_MID + negative) == 0;
    long enum_mixed = (long)(MODE_HIGH + negative);
    double folded = (double)(wide / 10u) + (double)(int)3.75;

    printf("%d %u %ld %.1f\n",
           enum_promoted,
           (unsigned int)narrow_sum,
           enum_mixed,
           folded);
    return 0;
}
