#include <stdio.h>

int main(void) {
    int acc = 0;
    int i = 0;

    do {
        int selector = (i * 3 + 2) % 5;

        switch (selector) {
            case 0:
                acc += 11;
                goto latch;

            case 1:
                acc += 17;
                break;

            case 2: {
                int j = 0;
                while (j < 4) {
                    ++j;
                    if (j == 2) {
                        continue;
                    }
                    acc += i + j;
                    if (j == 3) {
                        break;
                    }
                }
                break;
            }

            case 3:
                acc += 31;
                if (i < 3) {
                    goto latch;
                }
                break;

            default:
                acc += 43;
                break;
        }

        acc += 100 + i;

latch:
        acc += 6;
        ++i;
    } while (i < 7);

    printf("%d\n", acc);
    return 0;
}
