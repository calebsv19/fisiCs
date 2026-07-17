#include <stdio.h>

int main(void) {
    int selectors[] = {1, 2, 5, 0, 3, 2, 4};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        switch (selectors[i]) {
            case 0:
                acc += 5;
                break;

            case 1:
                acc += 10;
            case 2: {
                int j;

                for (j = 0; j < 4; ++j) {
                    if (((i + j) % 3) == 0) {
                        acc += j;
                        continue;
                    }
                    acc += 20 + j;
                    if (selectors[i] == 2 && j == 2) {
                        break;
                    }
                }

                if (i == 5) {
                    continue;
                }
                break;
            }

            default:
                acc += 1;
            case 3:
                acc += 30;
                continue;

            case 4:
                acc += 40;
                break;
        }

        acc += 1000 + i;
    }

    printf("%d\n", acc);
    return 0;
}
