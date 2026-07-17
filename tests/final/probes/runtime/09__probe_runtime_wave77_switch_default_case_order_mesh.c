#include <stdio.h>

int main(void) {
    int selectors[] = {2, 9, 0, 4, 3, 7, 1};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        switch (selectors[i]) {
            default:
                acc += 50 + i;
                if ((selectors[i] & 1) != 0) {
                    break;
                }
            case 0:
                acc += 3;
                break;

            case 1:
                acc += 10;
            case 2:
                acc += 20;
                if (i == 0) {
                    break;
                }
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
