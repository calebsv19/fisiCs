#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -9000:
            return 2;
        case -128:
        case -127:
            return 4;
        case 2048:
            return 6;
        case 50003:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-127, 50003, 0, -9000, 2048, -128, 77};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
