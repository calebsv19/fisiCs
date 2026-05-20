#include <stdio.h>

static int dispatch(int value) {
    switch ((signed char)value) {
        case -118:
            return 2;
        case -63:
            return 4;
        case 33:
            return 6;
        case 118:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {138, 289, 33, 630, 12};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
