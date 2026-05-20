#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -12001:
            return 2;
        case -5:
            return 4;
        case 8192:
            return 6;
        case 99001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {99001, 12, -12001, 8192, -5, 2};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
