#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -7001:
            return 2;
        case -5:
            return 4;
        case 73:
            return 6;
        case 15013:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-5, 15013, 0, -7001, 73, 99};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
