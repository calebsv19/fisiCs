#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -28000:
            return 2;
        case -4097:
        case -4096:
            return 4;
        case 6145:
            return 6;
        case 92003:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-4096, 92003, 0, -28000, 6145, -4097, 23};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
