#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 4; ++outer) {
        for (inner = 0; inner < 5; ++inner) {
            switch (outer + (inner * 2)) {
                case 0:
                    acc += 1;
                    break;
                case 2:
                    acc += 20;
                case 3:
                    acc += 200;
                    if (inner == 1) {
                        continue;
                    }
                    break;
                default:
                    acc += 1000;
                    if (outer == 3) {
                        break;
                    }
                    if (inner == 4) {
                        continue;
                    }
                    break;
            }

            acc += outer + inner + 6;
        }
    }

    printf("%d\n", acc);
    return 0;
}
