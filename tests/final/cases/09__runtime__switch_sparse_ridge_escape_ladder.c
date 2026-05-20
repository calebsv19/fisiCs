#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -12000:
            return 2;
        case -511:
        case -510:
            return 4;
        case 1536:
            return 6;
        case 64001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-510, 64001, 9, -12000, 1536, -511, 1};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
