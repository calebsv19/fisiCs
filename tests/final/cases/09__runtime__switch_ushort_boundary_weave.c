#include <stdio.h>

static int dispatch(int value) {
    switch ((unsigned short)value) {
        case 0u:
            return 1;
        case 1u:
            return 3;
        case 32768u:
            return 5;
        case 65535u:
            return 7;
        default:
            return 9;
    }
}

int main(void) {
    int values[] = {-1, 32768, 65536, 1, 9};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
