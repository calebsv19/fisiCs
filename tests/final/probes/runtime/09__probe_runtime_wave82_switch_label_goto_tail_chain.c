#include <stdio.h>

int main(void) {
    int selectors[] = {0, 3, 1, 2, 4, 3};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            case 0:
                acc += 10 + i;
                goto shared_tail;

            case 1:
                acc += 20;
                break;

            default:
                acc += 30 + selectors[i];
                if ((i & 1) != 0) {
                    goto shared_tail;
                }

            case 2:
                acc += 40 + i;
                break;

            case 4:
                acc += 50;
shared_tail:
                acc += 7 + i;
                break;
        }

        acc += 100 + i;
    }

    printf("%d\n", acc);
    return 0;
}
