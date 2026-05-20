#include <stdio.h>

static int dispatch(int value) {
    switch ((signed char)value) {
        case -128:
            return 9;
        case -1:
            return 7;
        case 0:
            return 5;
        case 127:
            return 3;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-128, 127, 255, 0, 8};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
