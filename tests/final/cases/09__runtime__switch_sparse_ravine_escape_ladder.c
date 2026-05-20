#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -16384:
            return 2;
        case -17:
            return 4;
        case 2048:
            return 6;
        case 70001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-17, 0, 70001, 2048, -16384, 13};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
