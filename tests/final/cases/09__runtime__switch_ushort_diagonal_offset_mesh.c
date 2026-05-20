#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned short)value) {
        case 11:
            return 2;
        case 2049:
            return 4;
        case 40001:
            return 6;
        case 65500:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {65500u, 2049u, 105537u, 40001u, 8u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
