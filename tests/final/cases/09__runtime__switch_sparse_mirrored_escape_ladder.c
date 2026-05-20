#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -9001:
            return 2;
        case -81:
            return 4;
        case 81:
            return 6;
        case 9001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {81, -81, 0, 9001, -9001, 3};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
