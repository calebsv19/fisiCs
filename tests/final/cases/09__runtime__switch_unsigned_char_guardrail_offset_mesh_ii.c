#include <stdio.h>

static int dispatch(unsigned int value) {
    switch ((unsigned char)value) {
        case 5:
            return 2;
        case 96:
            return 4;
        case 171:
            return 6;
        case 244:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {244u, 427u, 96u, 683u, 12u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
