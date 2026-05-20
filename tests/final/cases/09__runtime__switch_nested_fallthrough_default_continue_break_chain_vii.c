#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 4; ++outer) {
        for (inner = 0; inner < 5; ++inner) {
            switch ((outer * 2) + inner) {
                case 1:
                    acc += 10;
                case 2:
                    acc += 100;
                    if (inner == 2) {
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

            acc += outer + inner + 8;
        }
    }

    printf("%d\n", acc);
    return 0;
}
