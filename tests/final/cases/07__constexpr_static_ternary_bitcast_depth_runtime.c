#include <stdio.h>

enum {
    T0 = 9,
    T1 = ((unsigned)T0 << 4) | 3u,
    T2 = (int)(T1 ^ 0x2Au),
    T3 = (T2 & 31),
    T4 = (T3 ? (int)(unsigned)(T3 << 1) : (int)(unsigned)(T3 + 1))
};

static int g0 = (T4 ^ 7);
static int g1 = ((T2 & 1) ? (int)(unsigned)(T1 >> 2) : (int)(unsigned)(T1 >> 1));

int main(void) {
    printf("%d %d\n", g0, g1);
    return 0;
}

