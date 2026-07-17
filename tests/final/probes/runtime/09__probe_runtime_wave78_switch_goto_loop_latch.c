#include <stdio.h>

int main(void) {
    int selectors[] = {2, 0, 3, 1, 4, 2, 5};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        int local = i * 3;

        switch (selectors[i]) {
            case 0:
                acc += 11 + local;
                goto switch_tail;

            case 1:
                acc += 20;
                while (local < 8) {
                    local += 2;
                    if ((local & 3) == 0) {
                        continue;
                    }
                    acc += local;
                    if (local > 6) {
                        break;
                    }
                }
                break;

            case 2:
                acc += 30;
                if ((i & 1) == 0) {
                    goto loop_continue_latch;
                }
                break;

            case 3:
                acc += 40;
                goto switch_tail;

            default:
                acc += 50 + selectors[i];
                break;
        }

        acc += 100 + i;

switch_tail:
        acc += 7;

loop_continue_latch:
        acc += 3;
    }

    printf("%d\n", acc);
    return 0;
}
