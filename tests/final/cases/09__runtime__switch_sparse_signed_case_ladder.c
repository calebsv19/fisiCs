#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -7:
            return 11;
        case -1:
            return 7;
        case 0:
            return 5;
        case 9:
            return 3;
        case 42:
            return 1;
        default:
            return 0;
    }
}

int main(void) {
    int values[] = {-7, -1, 0, 9, 42, 5, -7};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
