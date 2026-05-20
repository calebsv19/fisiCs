#include <stdio.h>

static int dispatch(unsigned int value) {
    switch (value) {
        case 0u:
            return 2;
        case 1u:
            return 4;
        case 2147483648u:
            return 6;
        case 4294967295u:
            return 8;
        default:
            return 1;
    }
}

int main(void) {
    unsigned int values[] = {0u, 4294967295u, 2147483648u, 1u, 7u};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        acc = acc * 10 + dispatch(values[i]);
    }

    printf("%d\n", acc);
    return 0;
}
