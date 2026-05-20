#include <stdio.h>

int main(void) {
    int outer;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        int inner = outer - 1;
        while (inner <= outer) {
            switch (inner) {
                case -1:
                    acc += 1;
                    inner += 1;
                    continue;
                case 0:
                    acc += 10;
                    break;
                default:
                    acc += 100;
                    break;
            }

            acc += 1000;
            inner += 1;
        }
    }

    printf("%d\n", acc);
    return 0;
}
