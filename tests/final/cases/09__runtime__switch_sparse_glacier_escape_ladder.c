#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -28000:
            return 2;
        case -65:
            return 4;
        case 12288:
            return 6;
        case 150001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {12288, -65, 5, 150001, -28000, 1};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
