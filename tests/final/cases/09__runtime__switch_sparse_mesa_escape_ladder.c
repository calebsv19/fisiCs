#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -30000:
            return 2;
        case -1:
            return 4;
        case 4097:
            return 6;
        case 90000:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {4097, 12, -30000, 90000, -1, 5};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
