#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -42000:
            return 2;
        case -32769:
        case -32768:
            return 4;
        case 24577:
            return 6;
        case 123001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-32768, 123001, 0, -42000, 24577, -32769, 61};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
