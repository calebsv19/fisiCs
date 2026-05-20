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
                    break;
                case 6:
                    acc += 60;
                case 7:
                    acc += 70;
                    break;
                default:
                    acc += 1;
                    break;
            }

            acc += outer + inner + 12;
        }
    }

    printf("%d\n", acc);
    return 0;
}
