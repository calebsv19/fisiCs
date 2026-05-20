#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 6; ++inner) {
            switch ((outer * 5) + inner) {
                case 3:
                    acc += 10;
                case 4:
                    acc += 100;
                    if (inner == 4) {
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

            acc += outer + inner + 11;
        }
    }

    printf("%d\n", acc);
    return 0;
}
