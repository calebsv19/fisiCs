#include <stdio.h>

int main(void) {
    int outer;
    int inner;
    int acc = 0;

    for (outer = 0; outer < 2; ++outer) {
        for (inner = 0; inner < 5; ++inner) {
            switch ((outer * 5) + inner) {
                case 0:
                    acc += 5;
                    break;
                case 2:
                    acc += 20;
                    continue;
                case 4:
                    acc += 40;
                    break;
                case 5:
                    acc += 50;
                case 6:
                    acc += 60;
                    break;
                default:
                    acc += 1;
                    if (inner == 3) {
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
