#include <stdio.h>

enum {
    C0 = 3,
    C1 = (C0 * 5) + 2,
    LEN = (C1 ^ 7) + 4,
    CASE0 = LEN - 6,
    CASE1 = CASE0 + 3
};

static int arr[LEN];
static int s = (CASE1 * 2) - 1;

static int pick(int v) {
    switch (v) {
        case CASE0: return s - LEN;
        case CASE1: return s - CASE0;
        default: return -1;
    }
}

int main(void) {
    int n = (int)(sizeof(arr) / sizeof(arr[0]));
    printf("%d %d %d\n", n, s, pick(CASE1));
    return 0;
}
