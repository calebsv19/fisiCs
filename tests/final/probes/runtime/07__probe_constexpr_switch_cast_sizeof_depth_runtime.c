#include <stdio.h>

enum {
    B0 = (int)sizeof(int),
    B1 = (B0 << 3) - 1,
    B2 = (int)sizeof(long) + B1,
    KEY = (B2 & 63) - 2
};

static int route(int x) {
    switch (x) {
        case ((int)sizeof(char) + KEY - 1):
            return 11;
        case ((int)sizeof(short) + KEY):
            return 22;
        default:
            return -1;
    }
}

int main(void) {
    int x = ((int)sizeof(short) + KEY);
    printf("%d\n", route(x));
    return 0;
}
