#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -5000:
            return 2;
        case -64:
        case -63:
            return 4;
        case 511:
            return 6;
        case 12001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-63, 12001, 0, -5000, 511, -64, 7};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
