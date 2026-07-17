#include <stdio.h>

int main(void) {
    int selectors[] = {0, 2, 1, 3, 4, 2, 5};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        switch (selectors[i]) {
            case 0:
                acc += 3;
            case 1: {
                int j;

                for (j = 0; j < 3; ++j) {
                    if (((i + j) & 1) == 0) {
                        acc += j + 1;
                        continue;
                    }
                    acc += 10 + j;
                }

                if (selectors[i] == 1) {
                    continue;
                }
                break;
            }

            case 2:
                acc += 20;
            default:
                acc += 4;
                if (selectors[i] == 5) {
                    continue;
                }
                break;

            case 3:
                acc += 30;
                break;

            case 4:
                acc += 40;
                continue;
        }

        acc += 100 + i;
    }

    printf("%d\n", acc);
    return 0;
}
