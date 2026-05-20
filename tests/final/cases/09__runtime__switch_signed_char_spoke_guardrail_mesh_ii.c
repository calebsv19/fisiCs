#include <stdio.h>

static int dispatch(int value) {
    switch ((signed char)value) {
        case -114:
            return 2;
        case -57:
            return 4;
        case 57:
            return 6;
        case 114:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {142, 455, 57, 626, 15};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
