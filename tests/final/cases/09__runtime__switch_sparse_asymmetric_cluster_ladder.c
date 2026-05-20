#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -6001:
            return 2;
        case -33:
        case -32:
            return 4;
        case 777:
            return 6;
        case 25001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-32, 25001, 0, -6001, 777, -33, 8};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
