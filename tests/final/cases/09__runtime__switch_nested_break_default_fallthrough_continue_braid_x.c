#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 2; ++outer) {
        for (inner = 0; inner < 6; ++inner) {
            switch ((outer * 6) + inner) {
                case 1:
                    acc += 10;
                    break;
                case 4:
                    acc += 40;
                case 5:
                    acc += 50;
                    continue;
                case 7:
                    acc += 70;
                    break;
                default:
                    acc += 1;
                    break;
            }

            acc += outer + inner + 8;
        }
    }

    printf("%d\n", acc);
    return 0;
}
