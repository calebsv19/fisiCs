#include <stdio.h>

enum {
    C0 = (int)sizeof(int),
    C1 = ((int)sizeof(long) << 1) + C0,
    C2 = (int)(unsigned)(C1 ^ 0x35u),
    C3 = (C2 ? (int)(unsigned)(C2 >> 1) : (int)(unsigned)(C2 + 1)),
    C4 = ((C3 & 1) ? (C3 + (int)sizeof(char*)) : (C3 + (int)sizeof(short)))
};

static int g0 = C4 + (int)sizeof(char);
static int g1 = ((C2 & 3) ? (int)(unsigned)(C1 >> 1) : (int)(unsigned)(C1 >> 2));

int main(void) {
    printf("%d %d\n", g0, g1);
    return 0;
}
