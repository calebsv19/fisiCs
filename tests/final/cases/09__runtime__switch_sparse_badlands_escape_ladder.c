#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -9001:
            return 2;
        case -9:
            return 4;
        case 16384:
            return 6;
        case 110001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {110001, 7, -9001, 16384, -9, 0};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
