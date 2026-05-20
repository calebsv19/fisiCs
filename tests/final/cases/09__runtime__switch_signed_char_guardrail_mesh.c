#include <stdio.h>

static int dispatch(int value) {
    switch ((signed char)value) {
        case -120:
            return 2;
        case -31:
            return 4;
        case 42:
            return 6;
        case 119:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {136, 375, 42, 631, 9};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
