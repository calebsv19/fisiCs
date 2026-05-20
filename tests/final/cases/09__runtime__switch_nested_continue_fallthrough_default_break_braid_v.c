#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 6; ++inner) {
            switch ((outer * 3) + inner) {
                case 1:
                    acc += 10;
                    continue;
                case 3:
                    acc += 100;
                case 4:
                    acc += 1000;
                    if (outer == 1) {
                        break;
                    }
                    continue;
                default:
                    acc += 1;
                    if (inner == 5) {
                        break;
                    }
                    break;
            }

            acc += outer + inner + 7;
        }
    }

    printf("%d\n", acc);
    return 0;
}
