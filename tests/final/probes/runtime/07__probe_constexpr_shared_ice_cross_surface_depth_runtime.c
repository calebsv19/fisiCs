#include <stdio.h>

enum {
    B0 = 5,
    B1 = (B0 * B0) + (B0 << 1),
    B2 = ((B1 ^ 12) & 63) + 7,
    LEN = (B2 / 3) + 5,
    CASE0 = (LEN % 10) + 11,
    CASE1 = CASE0 + 2
};

static int a[LEN];
static int b[(LEN / 2) + 3];
static int s0 = CASE0 + LEN;
static int s1 = (CASE1 - (LEN % 5)) * 2;

static int route(int v) {
    switch (v) {
        case CASE0: return s0;
        case CASE1: return s1;
        default: return -1;
    }
}

int main(void) {
    int na = (int)(sizeof(a) / sizeof(a[0]));
    int nb = (int)(sizeof(b) / sizeof(b[0]));
    printf("%d %d %d %d\n", na, nb, route(CASE1), s0 - s1);
    return 0;
}
