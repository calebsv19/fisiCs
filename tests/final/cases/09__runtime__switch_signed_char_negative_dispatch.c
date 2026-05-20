#include <stdio.h>

static int dispatch(signed char value) {
    switch (value) {
        case -1:
            return 11;
        case 0:
            return 7;
        case 1:
            return 3;
        default:
            return 1;
    }
}

int main(void) {
    signed char values[] = {-1, 0, 1, 2, -1};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + (dispatch(values[i]) % 10);
    }

    printf("%d\n", acc);
    return 0;
}
