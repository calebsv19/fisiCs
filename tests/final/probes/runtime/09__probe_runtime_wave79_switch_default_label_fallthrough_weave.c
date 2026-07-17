#include <stdio.h>

int main(void) {
    int selectors[] = {2, 7, 1, 0, 3, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        int v = selectors[i];

        switch (v) {
            default:
                acc += 10 + v;
                if (v > 5) {
                    goto case_one_tail;
                }
                break;

            case 0:
                acc += 20;
                break;

            case 1:
                acc += 30;
case_one_tail:
                acc += i;
                break;

            case 2:
                acc += 40;
                if (i == 5) {
                    goto switch_done;
                }

            case 3:
                acc += 50 + i;
                break;
        }

        acc += 100 + i;

switch_done:
        acc += 3;
    }

    printf("%d\n", acc);
    return 0;
}
