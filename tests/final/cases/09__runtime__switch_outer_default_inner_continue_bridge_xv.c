#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 3; ++inner) {
            switch (outer) {
                case 0:
                    acc += 1;
                    switch (inner) {
                        case 0:
                            acc += 10;
                            break;
                        default:
                            acc += 20;
                        case 2:
                            acc += 30;
                            continue;
                    }
                    acc += 100;
                    break;
                case 1:
                    acc += 2;
                    break;
                default:
                    acc += 3;
                    switch (inner) {
                        case 0:
                            acc += 40;
                            break;
                        default:
                            acc += 50;
                            continue;
                    }
                    acc += 400;
                    break;
            }

            acc += outer + inner + 7;
        }
    }

    printf("%d\n", acc);
    return 0;
}
