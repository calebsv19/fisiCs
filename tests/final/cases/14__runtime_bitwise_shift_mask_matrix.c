#include <stdio.h>

int main(void) {
    unsigned x = 0xF0F0u;
    unsigned y = 0x0FF0u;

    unsigned r1 = (x & y) ^ (x | y);
    unsigned r2 = (x << 3) >> 4;
    int z = (int)((r1 & 0xFFu) + (r2 & 0xFFu));

    printf("%u %u %d\n", r1, r2, z);
    return 0;
}
