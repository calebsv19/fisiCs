#include <stdio.h>

enum Mode {
    MODE_NEG = -3,
    MODE_POS = 9
};

struct Bits {
    unsigned int low : 3;
    signed int high : 5;
    unsigned int flag : 1;
};

int main(void) {
    struct Bits bits = {7u, -6, 1u};
    signed char adjustment = -10;
    int signed_sum = bits.low + adjustment;
    unsigned int unsigned_sum = (unsigned int)bits.high + 4u;
    int mixed_compare = MODE_POS < unsigned_sum;
    int selected = bits.flag ? bits.high : (int)bits.low;

    printf("%d %u %d %d\n", signed_sum, unsigned_sum, mixed_compare, selected);
    return 0;
}
