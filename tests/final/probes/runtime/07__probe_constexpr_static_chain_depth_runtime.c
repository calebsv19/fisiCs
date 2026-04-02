#include <stdio.h>

enum {
    S0 = 5,
    S1 = (S0 << 2) + 1,
    S2 = (S1 ^ 13) + (int)sizeof(long),
    S3 = (S2 ? (S2 - 7) : (S2 + 7)),
    S4 = ((S3 & 31) + (int)sizeof(short))
};

static int g0 = S3 + ((int)sizeof(int) * 2);
static int g1 = (int)sizeof(char*) + S0 + (S1 % 9) + S4;

int main(void) {
    printf("%d %d\n", g0, g1);
    return 0;
}
