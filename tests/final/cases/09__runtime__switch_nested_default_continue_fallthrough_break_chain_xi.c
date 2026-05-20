#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 3; ++outer) {
        for (inner = 0; inner < 4; ++inner) {
            switch ((outer * 4) + inner) {
                case 0:
                    acc += 5;
                    break;
                case 2:
                    acc += 20;
                    continue;
                case 5:
                    acc += 50;
                case 6:
                    acc += 60;
                    break;
                case 9:
                    acc += 90;
                    break;
                default:
                    acc += 1;
                    break;
            }

            acc += outer + inner + 11;
        }
    }

    printf("%d\n", acc);
    return 0;
}
