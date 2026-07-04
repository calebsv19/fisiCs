#include <stdio.h>

int main(void) {
    int pairs[][2] = {{0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 2}, {2, 1}};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        int outer = pairs[i][0];
        int inner = pairs[i][1];

        switch (outer) {
            case 0:
                acc += 1;
                switch (inner) {
                    case 0:
                        acc += 10;
                        break;
                    default:
                        acc += 20;
                        break;
                }
            case 1:
                acc += 100;
                if (inner == 2) {
                    continue;
                }
                acc += 1000;
                break;
            default:
                acc += 10000;
                break;
        }

        acc += i + outer + inner;
    }

    printf("%d\n", acc);
    return 0;
}
