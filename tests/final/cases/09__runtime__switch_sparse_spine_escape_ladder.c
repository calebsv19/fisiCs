#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -32000:
            return 2;
        case -1025:
        case -1024:
            return 4;
        case 4097:
            return 6;
        case 81001:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-1024, 81001, 0, -32000, 4097, -1025, 17};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 7; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
