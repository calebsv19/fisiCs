#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -18000:
            return 2;
        case -24577:
        case -24576:
            return 4;
        case 32769:
            return 6;
        case 135003:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-24576, 135003, 0, -18000, 32769, -24577, 72};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
