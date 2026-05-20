#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 2; ++outer) {
        for (inner = 0; inner < 6; ++inner) {
            switch ((outer * 6) + inner) {
                case 0:
                    acc += 5;
                    break;
                case 3:
                    acc += 30;
                case 4:
                    acc += 40;
                    continue;
                case 8:
                    acc += 80;
                    break;
                default:
                    acc += 1;
                    break;
            }

            acc += outer + inner + 6;
        }
    }

    printf("%d\n", acc);
    return 0;
}
