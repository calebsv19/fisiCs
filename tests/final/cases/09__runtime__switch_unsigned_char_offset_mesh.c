#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned char)value) {
        case 3:
            return 2;
        case 64:
            return 4;
        case 129:
            return 6;
        case 250:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {250u, 515u, 64u, 385u, 0u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
