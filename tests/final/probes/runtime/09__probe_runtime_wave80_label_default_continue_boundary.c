#include <stdio.h>

int main(void) {
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        int selector = (i * 2 + 1) % 6;

        switch (selector) {
            case 1:
                acc += 5;
                if (i == 0) {
                    goto shared_tail;
                }

            case 3:
                acc += 7 + i;
                if (i == 3) {
                    continue;
                }
                break;

            default:
                acc += 11 + selector;
                if (selector == 5) {
                    goto shared_tail;
                }
                break;

            case 0: {
                int local = 13 + i;
                acc += local;
                break;
            }
        }

        acc += 20 + i;

shared_tail:
        acc += 2;
    }

    printf("%d\n", acc);
    return 0;
}
