#include <stdio.h>

int main(void) {
    int selectors[] = {3, 1, 4, 0, 2, 5, 1};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        int j;
        int local = i + 2;

        switch (selectors[i]) {
            case 0:
                acc += 11 + local;
                goto switch_tail;

            case 1:
                for (j = 0; j < 4; ++j) {
                    acc += local + j;
                    if (j == (i & 1)) {
                        continue;
                    }
                    if (j == 2) {
                        goto switch_tail;
                    }
                    acc += 7 + j;
                }
                break;

            case 2:
                acc += 29;
                if ((i & 1) == 0) {
                    continue;
                }
                break;

            case 3:
                acc += 37;
                break;

            default:
                acc += 41 + selectors[i];
                goto after_iteration;
        }

        acc += 100 + i;

switch_tail:
        acc += 5;

after_iteration:
        acc += 2;
    }

    printf("%d\n", acc);
    return 0;
}
