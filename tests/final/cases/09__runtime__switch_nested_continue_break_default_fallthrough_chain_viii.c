#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 4; ++outer) {
        for (inner = 0; inner < 5; ++inner) {
            switch ((outer * 3) + inner) {
                case 1:
                    acc += 10;
                    continue;
                case 4:
                    acc += 100;
                    break;
                default:
                    acc += 1000;
                    if (inner == 3) {
                        continue;
                    }
                    if (outer == 3) {
                        break;
                    }
                    break;
            }

            acc += outer + inner + 10;
        }
    }

    printf("%d\n", acc);
    return 0;
}
