#include <stdio.h>

int main(void) {
    int selectors[] = {1, 0, 3, 2, 4, 1};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            case 0: {
                int n = i + 2;
                int row[n];
                int j;

                for (j = 0; j < n; ++j) {
                    row[j] = acc + i + j;
                }

                acc += row[n - 1] % 23;
                goto switch_tail;
            }

            case 1:
                acc += 7;
            case 2: {
                int n = (i & 1) + 3;
                int row[n];
                int j;

                for (j = 0; j < n; ++j) {
                    row[j] = selectors[i] + i + j;
                }

                acc += row[0] + row[n - 1];
                if (i == 5) {
                    goto loop_tail;
                }
                break;
            }

            default:
                acc += 13;
                goto switch_tail;
        }

        acc += 100 + i;

switch_tail:
        acc += 5;

loop_tail:
        acc += 2;
    }

    printf("%d\n", acc);
    return 0;
}
