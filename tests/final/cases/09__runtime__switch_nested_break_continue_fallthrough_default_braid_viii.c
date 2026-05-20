#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 6; ++inner) {
            switch ((outer * 6) + inner) {
                case 1:
                    acc += 10;
                    break;
                case 6:
                    acc += 100;
                case 7:
                    acc += 1000;
                    if (inner == 1) {
                        continue;
                    }
                    break;
                default:
                    acc += 1;
                    if (outer == 2) {
                        break;
                    }
                    break;
            }

            acc += outer + inner + 13;
        }
    }

    printf("%d\n", acc);
    return 0;
}
