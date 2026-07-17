#include <stdio.h>

int main(void) {
    int acc = 0;
    int outer;

    for (outer = 0; outer < 5; ++outer) {
        int inner;

        for (inner = 0; inner < 4; ++inner) {
            int selector = (outer + inner) % 4;

            switch (selector) {
                case 0:
                    acc += outer + 3;
                    goto inner_tail;

                case 1:
                    acc += inner + 5;
                    continue;

                case 2:
                    acc += 11;
                    if (outer == 3) {
                        goto outer_tail;
                    }
                    break;

                default:
                    acc += 17;
                    break;
            }

            acc += 20 + inner;

inner_tail:
            acc += 2;
        }

        acc += 100 + outer;

outer_tail:
        acc += 7;
    }

    printf("%d\n", acc);
    return 0;
}
