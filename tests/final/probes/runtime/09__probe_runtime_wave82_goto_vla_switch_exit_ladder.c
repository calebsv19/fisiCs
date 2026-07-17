#include <stdio.h>

static int fold(int *values, int n) {
    int acc = 0;
    int i;

    for (i = 0; i < n; ++i) {
        acc += values[i] * (i + 2);
    }

    return acc;
}

int main(void) {
    int acc = 0;
    int n;

    for (n = 3; n < 8; ++n) {
        int selector = (n * 3) % 5;

        {
            int row[n];
            int i;

            for (i = 0; i < n; ++i) {
                row[i] = n - i + selector;
            }

            switch (selector) {
                case 0:
                    acc += fold(row, n);
                    goto loop_tail;

                case 1:
                    acc += row[0] + row[n - 1];
                    break;

                default:
                    acc += selector + n;
                    if ((n & 1) != 0) {
                        goto loop_tail;
                    }

                case 3:
                    acc += fold(row, n) / n;
                    break;
            }

            acc += 11 + n;
        }

loop_tail:
        acc += 2 * n;
    }

    printf("%d\n", acc);
    return 0;
}
