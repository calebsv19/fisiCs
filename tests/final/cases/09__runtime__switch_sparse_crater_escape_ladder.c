#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -15000:
            return 2;
        case -2049:
        case -2048:
            return 4;
        case 3072:
            return 6;
        case 70001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-2048, 70001, 0, -15000, 3072, -2049, 88};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
