#include <stdio.h>

int main(void) {
    int selectors[] = {0, 1, 2, 4, 0, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            case 0:
                acc += 1;
                goto bridge;

            case 1:
                acc += 10;
bridge:
                acc += 100;
                {
                    int j;
                    for (j = 0; j < 3; ++j) {
                        if (((i + j) & 1) != 0) {
                            acc += j;
                            continue;
                        }
                        acc += 5 + j;
                    }
                }
                break;

            default:
                acc += 2;
            case 2:
                acc += 20;
                if (i == 2) {
                    continue;
                }
                break;
        }

        acc += 1000 + i;
    }

    printf("%d\n", acc);
    return 0;
}
