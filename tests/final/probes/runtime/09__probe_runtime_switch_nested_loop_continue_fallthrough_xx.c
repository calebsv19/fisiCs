#include <stdio.h>

int main(void) {
    int selectors[] = {3, 1, 0, 3, 3, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        switch (selectors[i]) {
            case 3: {
                int j;
                acc += 30;
                for (j = 0; j < 3; ++j) {
                    switch ((i + j) & 3) {
                        case 0:
                            acc += 1;
                            continue;
                        case 1:
                            acc += 2;
                            break;
                        default:
                            acc += 3;
                            break;
                    }
                    acc += 10 + j;
                }
            }
            case 1:
                acc += 100;
                if (i == 4) {
                    continue;
                }
                break;

            default:
                acc += 7;
                break;
        }

        acc += 1000 + i;
    }

    printf("%d\n", acc);
    return 0;
}
