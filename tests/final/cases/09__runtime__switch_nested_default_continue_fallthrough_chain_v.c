#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 5; ++inner) {
            switch ((outer * 2) + inner) {
                case 0:
                    acc += 1;
                    continue;
                case 2:
                    acc += 20;
                case 3:
                    acc += 200;
                    if (outer == 1) {
                        continue;
                    }
                    break;
                default:
                    acc += 1000;
                    if (inner == 4) {
                        continue;
                    }
                    break;
            }

            acc += outer + inner + 4;
        }
    }

    printf("%d\n", acc);
    return 0;
}
