#include <stdio.h>

int main(void) {
    int selectors[] = {2, 1, 0, 3, 2, 4, 1};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        int j;

        for (j = 0; j < 4; ++j) {
            switch ((selectors[i] + j) % 5) {
                case 0:
                    acc += 1;
                    break;

                case 1:
                    acc += 10;
                    continue;

                case 2:
                    acc += 20;
                case 3:
                    acc += j;
                    if (j == 2) {
                        break;
                    }
                    continue;

                default:
                    acc += 30;
                    break;
            }

            acc += 100 + j;
            if (selectors[i] == 4) {
                break;
            }
        }

        acc += 1000 + i;
    }

    printf("%d\n", acc);
    return 0;
}
