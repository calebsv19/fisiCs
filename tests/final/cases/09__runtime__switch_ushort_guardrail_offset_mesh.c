#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned short)value) {
        case 42:
            return 2;
        case 4096:
            return 4;
        case 32768:
            return 6;
        case 65530:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {65530u, 42u, 32768u, 69632u, 1u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
