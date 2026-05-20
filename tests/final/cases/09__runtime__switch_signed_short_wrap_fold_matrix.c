#include <stdio.h>

static int dispatch(int value) {
    switch ((short)value) {
        case -32768:
            return 7;
        case -2:
            return 5;
        case 2:
            return 3;
        case 32767:
            return 1;
        default:
            return 9;
    }
}

int main(void) {
    int values[] = {32768, 65534, 2, 32767, 19};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
