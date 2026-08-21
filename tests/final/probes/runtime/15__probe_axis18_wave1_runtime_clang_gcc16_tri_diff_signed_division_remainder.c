#include <stdio.h>

static unsigned axis18_encode(int dividend, int divisor) {
    int quotient = dividend / divisor;
    int remainder = dividend % divisor;

    return (unsigned)(quotient + 32) * 97u + (unsigned)(remainder + 32);
}

int main(void) {
    unsigned first = axis18_encode(-31, 7) ^ axis18_encode(31, -7);
    unsigned second = axis18_encode(-29, -5) * 257u + axis18_encode(29, 5);

    printf("axis18-div=%u,%u\n", first, second);
    return 0;
}
