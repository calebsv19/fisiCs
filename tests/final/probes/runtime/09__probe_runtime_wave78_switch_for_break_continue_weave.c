#include <stdio.h>

int main(void) {
    int selectors[] = {1, 2, 4, 0, 3, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            case 0:
                acc += 13;
                break;

            case 1:
            case 2: {
                int j;
                for (j = 0; j < 5; ++j) {
                    if (j == selectors[i]) {
                        continue;
                    }
                    acc += i + j;
                    if (j == 3) {
                        break;
                    }
                }
                if (selectors[i] == 2 && (i & 1) == 1) {
                    continue;
                }
                break;
            }

            case 3:
                acc += 33;
                goto after_switch;

            default:
                acc += 44;
                break;
        }

        acc += 100 + i;

after_switch:
        acc += 6;
    }

    printf("%d\n", acc);
    return 0;
}
