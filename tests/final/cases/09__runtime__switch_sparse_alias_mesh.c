#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -1000:
            return 1;
        case -3:
        case 0:
            return 3;
        case 5:
            return 5;
        case 42:
        case 1000:
            return 7;
        default:
            return 9;
    }
}

int main(void) {
    int values[] = {42, -3, 8, 1000, 0, -1000, 5};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
