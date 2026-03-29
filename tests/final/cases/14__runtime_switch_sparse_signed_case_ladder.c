#include <stdio.h>

static int dispatch(int x) {
    switch (x) {
        case -1000:
            return 7;
        case -3:
            return -5;
        case 0:
            return 11;
        case 9:
            return 4;
        case 12345:
            return -9;
        default:
            return x & 3;
    }
}

int main(void) {
    int values[8] = {-1000, -3, -2, 0, 9, 10, 12345, 12346};
    int sum = 0;
    int xorv = 0;

    for (int i = 0; i < 8; ++i) {
        int r = dispatch(values[i]);
        sum += r;
        xorv ^= (r + i);
    }

    printf("%d %d\n", sum, xorv);
    return 0;
}

