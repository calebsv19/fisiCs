#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 4; ++inner) {
            switch (outer) {
                case 0:
                    acc += 1;
                    switch (inner) {
                        case 0:
                            acc += 10;
                            break;
                        default:
                            acc += 20;
                            continue;
                    }
                    acc += 100;
                    break;
                case 1:
                    acc += 2;
                    switch (inner) {
                        case 1:
                            acc += 30;
                            break;
                        default:
                            acc += 40;
                            break;
                    }
                    break;
                default:
                    acc += 3;
                    break;
            }

            acc += outer + inner;
        }
    }

    printf("%d\n", acc);
    return 0;
}
