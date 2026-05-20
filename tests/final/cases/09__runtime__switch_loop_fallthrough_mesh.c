#include <stdio.h>

static int score(int outer, int inner) {
    int acc = 0;

    switch (outer) {
        case 0:
            acc += 1;
            break;
        case 1:
            acc += 2;
            switch (inner) {
                case 0:
                    acc += 3;
                    break;
                case 1:
                    acc += 4;
                default:
                    acc += 5;
                    break;
            }
            break;
        default:
            acc += 6;
            break;
    }

    return acc;
}

int main(void) {
    int pairs[][2] = {{0, 0}, {1, 0}, {1, 1}, {2, 7}};
    int acc = 0;
    int i = 0;

    for (i = 0; i < 4; ++i) {
        acc = acc * 10 + score(pairs[i][0], pairs[i][1]);
    }

    printf("%d\n", acc);
    return 0;
}
