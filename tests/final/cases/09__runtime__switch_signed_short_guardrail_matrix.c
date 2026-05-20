#include <stdio.h>

static int dispatch(int value) {
    switch ((short)value) {
        case -22222:
            return 2;
        case -15:
            return 4;
        case 15:
            return 6;
        case 22222:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {43314, 22222, 65521, 15, 77};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
