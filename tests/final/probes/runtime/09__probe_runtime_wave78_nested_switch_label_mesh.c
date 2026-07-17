#include <stdio.h>

int main(void) {
    int acc = 0;
    int outer;

    for (outer = 0; outer < 5; ++outer) {
        int selector = outer % 3;

        switch (selector) {
            case 0:
                acc += 5;
                goto nested_entry;

            case 1: {
                int inner = outer + 1;
                switch (inner & 3) {
                    case 0:
                        acc += 17;
                        break;
                    case 1:
nested_entry:
                        acc += 23 + outer;
                        if (outer == 3) {
                            goto outer_tail;
                        }
                        break;
                    default:
                        acc += 31;
                        continue;
                }
                break;
            }

            default:
                acc += 41;
                break;
        }

        acc += 100 + outer;

outer_tail:
        acc += 9;
    }

    printf("%d\n", acc);
    return 0;
}
