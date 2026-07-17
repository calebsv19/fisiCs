#include <stdio.h>

int main(void) {
    int acc = 0;
    int outer;

    for (outer = 0; outer < 4; ++outer) {
        int selector = outer - 1;

        switch (selector) {
            case -1: {
                int j;
                for (j = 0; j < 3; ++j) {
                    acc += j;
                    if (j == 1) {
                        continue;
                    }
                    acc += 10;
                }
                acc += 20;
                break;
            }

            case 0:
                acc += 30;
                continue;

            case 1: {
                int k = 0;
                do {
                    acc += 40 + k;
                    if (k == 0) {
                        ++k;
                        continue;
                    }
                    ++k;
                } while (k < 2);
                acc += 50;
                break;
            }

            default:
                acc += 60;
                break;
        }

        acc += 100 + outer;
    }

    printf("%d\n", acc);
    return 0;
}
