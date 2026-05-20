#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned short)value) {
        case 17:
            return 2;
        case 513:
            return 4;
        case 32770:
            return 6;
        case 65535:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {65535u, 17u, 66049u, 32770u, 9u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
