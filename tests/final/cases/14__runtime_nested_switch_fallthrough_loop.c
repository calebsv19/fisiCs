#include <stdio.h>

int main(void) {
    int acc = 0;
    int hits = 0;

    for (int i = 0; i < 6; ++i) {
        switch (i & 3) {
            case 0:
                acc += 1;
                /* intentional fallthrough */
            case 1:
                acc += 10;
                if (i == 1) {
                    continue;
                }
                break;
            case 2: {
                int inner = i >> 1;
                switch (inner) {
                    case 1:
                        acc += 100;
                        break;
                    default:
                        acc += 200;
                        break;
                }
                if (i == 2) {
                    break;
                }
                acc += 1000;
                break;
            }
            default:
                acc += 10000;
                goto done_iteration;
        }

        hits += i;
    done_iteration:
        acc += 3;
    }

    printf("%d %d\n", acc, hits);
    return 0;
}
