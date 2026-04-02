#include <stdio.h>

enum {
    C0 = 4,
    C1 = (C0 * 5) + 3,
    C2 = (C1 ^ 18) + (C0 << 2),
    C3 = (C2 & 31) + 7,
    C4 = (C3 << 1) - (C1 % 6),
    K0 = (C4 % 17) + 11,
    K1 = (K0 ^ 9),
    K2 = (K1 - 3)
};

static int dispatch(int key) {
    switch (key) {
        case K0:
            return 101;
        case K1:
            return 202;
        case K2:
            return 303;
        default:
            return -1;
    }
}

int main(void) {
    int key = K1;
    printf("%d\n", dispatch(key));
    return 0;
}
