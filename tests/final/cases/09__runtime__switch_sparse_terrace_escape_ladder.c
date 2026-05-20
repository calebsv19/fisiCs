#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -24000:
            return 2;
        case -16385:
        case -16384:
            return 4;
        case 16385:
            return 6;
        case 111003:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-16384, 111003, 0, -24000, 16385, -16385, 45};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
