#include <stdio.h>

static int dispatch(int value) {
    switch ((short)value) {
        case -12345:
            return 2;
        case -11:
            return 4;
        case 11:
            return 6;
        case 12345:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {53191, 12345, 65525, 11, 99};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
