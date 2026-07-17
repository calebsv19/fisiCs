#include <stdio.h>

static int consume(int *p, int n) {
    int total = 0;
    int i;
    for (i = 0; i < n; ++i) {
        total += p[i] * (i + 1);
    }
    return total;
}

int main(void) {
    int acc = 0;
    int n;

    for (n = 2; n < 7; ++n) {
        int selector = n % 4;

        {
            int row[n];
            int i;
            for (i = 0; i < n; ++i) {
                row[i] = n + i;
            }

            switch (selector) {
                case 0:
                    acc += consume(row, n);
                    goto loop_latch;

                case 1:
                    acc += row[n - 1];
                    break;

                default:
                    acc += row[0] + selector;
                    if ((n & 1) != 0) {
                        goto loop_latch;
                    }

                case 2:
                    acc += consume(row, n) / n;
                    break;
            }

            acc += 30 + n;
        }

loop_latch:
        acc += 3 * n;
    }

    printf("%d\n", acc);
    return 0;
}
