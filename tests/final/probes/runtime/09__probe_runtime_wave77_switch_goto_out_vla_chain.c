#include <stdio.h>

int main(void) {
    int selectors[] = {1, 0, 3, 2, 4, 1};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            case 0: {
                int n = i + 3;
                int row[n];
                int j;

                for (j = 0; j < n; ++j) {
                    row[j] = acc + i + j;
                }

                acc += row[n - 1] % 37;
                goto after_switch;
            }

            case 1:
                acc += 7;
            case 2: {
                int n = (i & 1) + 2;
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

            case 3: {
                int n = 4;
                int row[n];
                row[0] = acc + 3;
                row[3] = row[0] + i;
                acc += row[3] % 41;
                goto after_switch;
            }

            default:
                acc += 13;
                goto after_switch;
        }

        acc += 100 + i;

after_switch:
        acc += 5;

loop_tail:
        acc += 2;
    }

    printf("%d\n", acc);
    return 0;
}
