#include <stdio.h>

int main(void) {
    int selectors[] = {0, 3, 1, 4, 2, 5};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        int j;

        switch (selectors[i]) {
            case 0:
                acc += 3;
            case 1:
                for (j = 0; j < 4; ++j) {
                    acc += j + 1;
                    if (((i + j) & 1) != 0) {
                        continue;
                    }
                    acc += 10 + j;
                }
                if (selectors[i] == 1) {
                    continue;
                }
                break;

            case 2:
                acc += 20;
            default:
                for (j = 0; j < 3; ++j) {
                    acc += 30 + j;
                    if (j == (i % 3)) {
                        break;
                    }
                }
                if (selectors[i] == 5) {
                    continue;
                }
                break;

            case 3:
                acc += 300;
                break;

            case 4:
                acc += 400;
                continue;
        }

        acc += 1000 + i;
    }

    printf("%d\n", acc);
    return 0;
}
