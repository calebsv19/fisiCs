#include <stdio.h>

int main(void) {
    int selectors[] = {0, 2, 1, 3, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        switch (selectors[i]) {
            case 0: {
                int n = i + 2;
                int row[n];
                int j;

                for (j = 0; j < n; ++j) {
                    row[j] = (i + 1) * 10 + j;
                }

                acc += row[n - 1];
                goto after_switch;
            }

            case 1:
                acc += 11;
                break;

            default:
                acc += 3;
            case 2: {
                int n = (i & 1) + 3;
                int row[n];
                int j;

                for (j = 0; j < n; ++j) {
                    row[j] = selectors[i] + i + j;
                }

                acc += row[0] + row[n - 1];
                if (i == 3) {
                    goto after_switch;
                }
                break;
            }
        }

        acc += 100 + i;

after_switch:
        acc += 7;
    }

    printf("%d\n", acc);
    return 0;
}
