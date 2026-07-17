#include <stdio.h>

int main(void) {
    signed char sc = -1;
    unsigned char uc = 1;
    unsigned short us = 65535u;
    signed char delta = -2;
    unsigned int big = 4000000000u;
    int neg = -1;

    int promoted_compare = (sc < uc);
    int promoted_sum = us + delta;
    int unsigned_compare = (neg < big);
    int cast_chain = (int)((double)(unsigned char)200 / 4.0);
    unsigned int wrap = (unsigned int)3 - (int)5;

    printf("%d %d %d %d %u\n",
           promoted_compare,
           promoted_sum,
           unsigned_compare,
           cast_chain,
           wrap);
    return 0;
}
