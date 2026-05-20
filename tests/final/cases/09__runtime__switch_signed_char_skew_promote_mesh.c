#include <stdio.h>

static int dispatch(int value) {
    switch ((signed char)value) {
        case -101:
            return 2;
        case -7:
            return 4;
        case 5:
            return 6;
        case 97:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {155, 97, 249, 5, 400};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
