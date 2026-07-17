#include <stdio.h>

int main(void) {
    int outer_values[] = {2, 0, 3, 1, 4, 2};
    int inner_values[] = {0, 3, 1, 2, 4, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        int outer = outer_values[i];
        int inner = inner_values[i];

        switch (outer) {
            case 0:
                acc += 10;
                switch (inner) {
                    case 1:
                        acc += 20;
                        break;

                    default:
                        acc += 30 + inner;
                        goto nested_done;

                    case 3:
                        acc += 40;
nested_done:
                        acc += i;
                        break;
                }
                acc += 5;
                break;

            case 2:
                acc += 50 + i;
                if (inner == 2) {
                    goto outer_done;
                }
                break;

            default:
                acc += 60 + outer;

            case 4:
                acc += 70 + inner;
                break;
        }

        acc += 100 + i;
outer_done:
        acc += 9;
    }

    printf("%d\n", acc);
    return 0;
}
