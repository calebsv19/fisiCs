#include <stdio.h>

static int dispatch(int value) {
    switch ((short)value) {
        case -32768:
            return 9;
        case -1:
            return 7;
        case 0:
            return 5;
        case 32767:
            return 3;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {32768, -1, 65535, 0, 12};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
