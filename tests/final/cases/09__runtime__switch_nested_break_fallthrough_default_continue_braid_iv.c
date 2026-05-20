#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 4; ++outer) {
        for (inner = 0; inner < 4; ++inner) {
            switch (outer + inner) {
                case 1:
                    acc += 10;
                    break;
                case 2:
                    acc += 100;
                case 3:
                    acc += 1000;
                    if (inner == 2) {
                        continue;
                    }
                    break;
                default:
                    acc += 1;
                    if (outer == 3) {
                        continue;
                    }
                    break;
            }

            acc += outer + inner + 5;
        }
    }

    printf("%d\n", acc);
    return 0;
}
