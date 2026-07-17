#include <stdio.h>

int main(void) {
    int selectors[] = {1, 3, 2, 0, 4, 2, 5};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        int j;

        switch (selectors[i]) {
            case 0:
                acc += 10 + i;
                break;

            case 1:
                acc += 20;

            case 2:
                for (j = 0; j < 3; ++j) {
                    if ((i + j) & 1) {
                        acc += 2;
                        continue;
                    }
                    acc += 5 + j;
                }
                if (i == 5) {
                    continue;
                }
                break;

            default:
                acc += 30 + selectors[i];
                if (selectors[i] > 4) {
                    continue;
                }

            case 4:
                acc += 40 + i;
                break;
        }

        acc += 100 + i;
    }

    printf("%d\n", acc);
    return 0;
}
