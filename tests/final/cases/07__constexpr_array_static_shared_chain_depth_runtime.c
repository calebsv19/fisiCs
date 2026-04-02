#include <stdio.h>

enum {
    A0 = 7,
    A1 = (A0 << 3) + 5,
    A2 = (A1 ^ 41) + (A0 * 3),
    A3 = ((A2 & 63) ? (A2 - 9) : (A2 + 9)),
    A4 = (A3 >> 1) + (A1 % 7),
    LEN = (A4 % 11) + 10
};

static int arr[LEN];
static int s0 = LEN + (A2 & 15);
static int s1 = (A3 ^ A0) - (int)sizeof(short);

int main(void) {
    int n = (int)(sizeof(arr) / sizeof(arr[0]));
    printf("%d %d %d\n", n, s0, s1);
    return 0;
}
