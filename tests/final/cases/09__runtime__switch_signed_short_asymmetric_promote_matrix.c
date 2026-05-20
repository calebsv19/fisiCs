#include <stdio.h>

static int dispatch(int value) {
    switch ((short)value) {
        case -30000:
            return 8;
        case -7:
            return 6;
        case 9:
            return 4;
        case 30000:
            return 2;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {35536, 65529, 9, 30000, 5};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
