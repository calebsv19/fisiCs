#include <stdio.h>

static int score(int outer, int inner) {
    int acc = 0;

    switch (outer) {
        case 0:
            acc += 1;
            switch (inner) {
                case 0:
                    acc += 10;
                    break;
                case 1:
                    acc += 20;
                    break;
                default:
                    acc += 30;
                    break;
            }
        case 1:
            acc += 100;
            if ((inner & 1) != 0) {
                break;
            }
        default:
            acc += 1000;
            break;
    }

    return acc;
}

int main(void) {
    int pairs[][2] = {{0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {2, 0}};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        acc += score(pairs[i][0], pairs[i][1]);
    }

    printf("%d\n", acc);
    return 0;
}
