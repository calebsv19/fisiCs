#include <stdio.h>

int main(void) {
    int selectors[] = {2, 0, 3, 1, 4, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            default:
                acc += 3;
            case 0: {
                int n = (i & 1) + 2;
                int row[n];
                int j;

                for (j = 0; j < n; ++j) {
                    row[j] = acc + i + j + 1;
                }

                acc += row[n - 1] % 17;
                if (selectors[i] == 3) {
                    goto after_switch;
                }
                break;
            }

            case 1:
                acc += 11;
                continue;

            case 2:
                acc += 20;
                if (i == 5) {
                    goto after_switch;
                }
                break;
        }

        acc += 100 + i;

after_switch:
        acc += 7;
    }

    printf("%d\n", acc);
    return 0;
}
