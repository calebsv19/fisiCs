#include <stdio.h>

static int dispatch(int value) {
    switch ((signed char)value) {
        case -110:
            return 2;
        case -45:
            return 4;
        case 17:
            return 6;
        case 110:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {146, 273, 17, 622, 3};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
