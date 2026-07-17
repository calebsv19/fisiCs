#include <stdio.h>

static int step(int selector, int seed) {
    int total = seed;

    switch (selector) {
        case 0: {
            int left = seed + 4;
            total += left;
            break;
        }
        case 1: {
            int middle = seed * 2;
            total += middle;
            break;
        }
        default: {
            int right = seed - 3;
            total += right;
            break;
        }
    }

    return total;
}

int main(void) {
    int selectors[] = {0, 1, 2, 1, 0, 3};
    int total = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        total += step(selectors[i], i + 5);
    }

    printf("%d\n", total);
    return 0;
}
