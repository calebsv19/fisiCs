#include <stdio.h>

static int dispatch(int value) {
    switch (value) {
        case -4097:
            return 2;
        case -1:
            return 4;
        case 257:
            return 6;
        case 16385:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {257, -4097, 0, 16385, -1, 4};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 6; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
