#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned short)value) {
        case 33:
            return 2;
        case 1025:
            return 4;
        case 32767:
            return 6;
        case 65000:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {65000u, 1025u, 131097u, 33u, 8u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
