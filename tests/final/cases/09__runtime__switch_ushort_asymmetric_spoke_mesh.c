#include <stdio.h>

static int dispatch(int value) {
    switch ((unsigned short)value) {
        case 0u:
            return 2;
        case 13u:
            return 4;
        case 40001u:
            return 6;
        case 65535u:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-1, 40001, 65536, 13, 21};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
