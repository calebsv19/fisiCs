#include <stdio.h>

static int classify(int value) {
    switch ((unsigned char)value) {
        case 255:
            return 9;
        case 0:
            return 4;
        default:
            return 1;
    }
}

int main(void) {
    int values[] = {-1, 0, 1, 256};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 4; ++i) {
        switch (i & 1) {
            case 0:
                acc += classify(values[i]);
                break;
            default:
                acc = acc * 2 + classify(values[i]);
                break;
        }
    }

    printf("%d\n", acc);
    return 0;
}
