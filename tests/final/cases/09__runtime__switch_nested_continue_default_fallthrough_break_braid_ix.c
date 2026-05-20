#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 4; ++inner) {
            switch ((outer * 4) + inner) {
                case 1:
                    acc += 10;
                    continue;
                case 4:
                    acc += 40;
                case 5:
                    acc += 50;
                    break;
                case 9:
                    acc += 90;
                    break;
                default:
                    acc += 1;
                    if (outer == 2 && inner == 3) {
                        break;
                    }
                    break;
            }

            acc += outer + inner + 9;
        }
    }

    printf("%d\n", acc);
    return 0;
}
