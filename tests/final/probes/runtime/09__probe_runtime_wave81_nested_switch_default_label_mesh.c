#include <stdio.h>

int main(void) {
    int outer_values[] = {0, 2, 4, 1, 3};
    int inner_values[] = {1, 0, 3, 2, 4};
    int acc = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        int outer = outer_values[i];
        int inner = inner_values[i];

        switch (outer) {
            case 0:
                acc += 10;
                switch (inner) {
                    default:
                        acc += 100 + inner;
                        goto nested_tail;
                    case 1:
                        acc += 20;
nested_tail:
                        acc += i;
                        break;
                }
                acc += 3;
                break;

            default:
                acc += 30 + outer;
                if (inner == 2) {
                    goto outer_tail;
                }

            case 2:
                acc += 40 + i;
                break;

            case 4:
                acc += 50;
                switch (inner) {
                    case 3:
                        acc += 60;
                    default:
                        acc += 70 + i;
                        break;
                }
                break;
        }

        acc += 200 + i;
outer_tail:
        acc += 5;
    }

    printf("%d\n", acc);
    return 0;
}
