#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -2049:
            return 2;
        case -17:
            return 4;
        case 33:
            return 6;
        case 4097:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {33, -17, 0, 4097, -2049, 2};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
