#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -36000:
            return 2;
        case -8193:
        case -8192:
            return 4;
        case 12289:
            return 6;
        case 99001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-8192, 99001, 0, -36000, 12289, -8193, 31};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
