#include <stdio.h>

int main(void) {
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        int selector = (i * 5 + 1) % 4;

        switch (selector) {
            case 0: {
                int step = i + 2;
                int j;

                for (j = 0; j < 3; ++j) {
                    acc += step + j;
                    if (((j + i) & 1) == 0) {
                        continue;
                    }
                    acc += 10 + j;
                }
                break;
            }

            case 1: {
                int step = i + 4;
                acc += step;
                if ((i & 1) == 0) {
                    continue;
                }
                acc += 31;
                break;
            }

            case 2: {
                int j;
                for (j = 0; j < 4; ++j) {
                    if (j == 2) {
                        break;
                    }
                    acc += i + j;
                }
                break;
            }

            default: {
                int step = selector + i;
                acc += step * 3;
                break;
            }
        }

        acc += 100 + i;
    }

    printf("%d\n", acc);
    return 0;
}
