#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned short)value) {
        case 7:
            return 2;
        case 8192:
            return 4;
        case 45000:
            return 6;
        case 65510:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {65510u, 7u, 110536u, 45000u, 123u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
