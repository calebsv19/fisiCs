#include <stdio.h>

int main(void) {
    int selectors[] = {4, 1, 3, 0, 2, 4};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            default: {
                int base = selectors[i] + i;
                acc += base;
                if ((base & 1) != 0) {
                    goto odd_tail;
                }
            }

            case 0: {
                int zero = 10 + i;
                acc += zero;
                break;
            }

            case 1: {
                int one = 20 + i;
                acc += one;
odd_tail:
                acc += 3 * i;
                break;
            }

            case 2:
                acc += 40 + i;

            case 3: {
                int three = 50 + i;
                acc += three;
                break;
            }
        }

        acc += 100 + i;
    }

    printf("%d\n", acc);
    return 0;
}
