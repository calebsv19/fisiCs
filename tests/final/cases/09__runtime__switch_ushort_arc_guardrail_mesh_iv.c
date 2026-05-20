#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned short)value) {
        case 65:
            return 2;
        case 2049:
            return 4;
        case 30000:
            return 6;
        case 64000:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {64000u, 2049u, 95536u, 65u, 11u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
