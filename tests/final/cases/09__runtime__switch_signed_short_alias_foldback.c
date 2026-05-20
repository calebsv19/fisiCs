#include <stdio.h>

static int dispatch(int value) {
    switch ((short)value) {
        case -32768:
            return 8;
        case -1:
            return 6;
        case 1:
            return 4;
        case 32767:
            return 2;
        default:
            return 9;
    }
}

int main(void) {
    int values[] = {65535, 32767, 32768, 1, 12};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
