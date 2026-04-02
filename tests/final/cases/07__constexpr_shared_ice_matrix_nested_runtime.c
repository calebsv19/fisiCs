#include <stdio.h>

enum {
    A0 = 9,
    A1 = (A0 << 2) - 5,
    A2 = ((A1 & 15) ? A1 : A0) + 3,
    LEN = (A2 % 9) + 12,
    CASE0 = LEN - 4,
    CASE1 = CASE0 + ((A0 & 1) ? 2 : 4)
};

static int arr[LEN];
static int s = (CASE1 * 3) - (LEN >> 1);

static int choose(void) {
    switch (CASE0) {
        case CASE0: return s - LEN;
        case CASE1: return s - CASE0;
        default: return -1;
    }
}

int main(void) {
    int n = (int)(sizeof(arr) / sizeof(arr[0]));
    printf("%d %d %d\n", n, s, choose());
    return 0;
}
