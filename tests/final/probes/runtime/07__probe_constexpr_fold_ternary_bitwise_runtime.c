#include <stdio.h>

enum {
    A = (1 << 3),
    B = ((A & 7) ? (A | 3) : 0),
    C = ((B ^ 5) + (A >> 1)),
    D = ((C > 8) ? (C - 2) : (C + 2))
};

static int g_arr[D + 4];

int main(void) {
    int n = (int)(sizeof(g_arr) / sizeof(g_arr[0]));
    int out = A + B + C + D + n;
    printf("%d\n", out);
    return 0;
}
