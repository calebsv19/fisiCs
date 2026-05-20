#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -22000:
            return 2;
        case -33:
            return 4;
        case 3072:
            return 6;
        case 120000:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {3072, -33, 0, 120000, -22000, 19};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
