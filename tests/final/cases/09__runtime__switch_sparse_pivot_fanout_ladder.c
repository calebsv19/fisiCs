#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -4000:
            return 2;
        case -3999:
        case 7:
            return 4;
        case 999:
            return 6;
        case 41000:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {7, 41000, -1, -4000, 999, -3999, 33};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
