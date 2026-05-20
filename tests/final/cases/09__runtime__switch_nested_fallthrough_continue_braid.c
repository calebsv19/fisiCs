#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 2; ++outer) {
        for (inner = 0; inner < 4; ++inner) {
            switch (outer + inner) {
                case 0:
                    acc += 1;
                    continue;
                case 1:
                    acc += 10;
                case 2:
                    acc += 100;
                    break;
                default:
                    acc += 1000;
                    if (outer == 1) {
                        continue;
                    }
                    break;
            }

            acc += outer;
        }
    }

    printf("%d\n", acc);
    return 0;
}
