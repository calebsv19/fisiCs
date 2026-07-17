#include <stdio.h>

int main(void) {
    int acc = 0;
    int outer;

    for (outer = 0; outer < 5; ++outer) {
        int inner = 0;

        while (inner < 4) {
            int selector = (outer * 3 + inner) % 5;

            switch (selector) {
                case 0:
                    acc += 7 + outer;
                    goto inner_latch;

                case 1:
                    acc += 11 + inner;
                    break;

                case 2:
                    acc += 13;
                    switch ((outer + inner) & 1) {
                        case 0:
                            acc += 17;
                            goto outer_latch;
                        default:
                            acc += 19;
                            break;
                    }
                    break;

                default:
                    acc += 23 + selector;
                    if (inner == 3) {
                        goto outer_latch;
                    }
                    break;
            }

            acc += 30 + inner;

inner_latch:
            acc += 2;
            ++inner;
        }

        acc += 100 + outer;

outer_latch:
        acc += 5;
    }

    printf("%d\n", acc);
    return 0;
}
