#include <stdio.h>

enum {
    B0 = 6,
    B1 = (B0 << 2) + 3,
    B2 = (B1 ^ 25),
    B3 = (B2 & 31),
    K0 = (B3 ? ((B3 & 1) ? (B3 + 5) : (B3 + 7)) : 9),
    K1 = (K0 ^ 12),
    K2 = (K1 - 4)
};

static int dispatch(int key) {
    switch (key) {
        case K0: return 101;
        case K1: return 202;
        case K2: return 303;
        default: return -1;
    }
}

int main(void) {
    int key = K1;
    printf("%d\n", dispatch(key));
    return 0;
}
