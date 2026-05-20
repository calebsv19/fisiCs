#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -11011:
            return 2;
        case -121:
            return 4;
        case 1331:
            return 6;
        case 17017:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {1331, -121, 0, 17017, -11011, 6};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
