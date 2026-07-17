#include <stdio.h>

int main(void) {
    int selectors[] = {2, 5, 1, 4, 0, 3, 2};
    int acc = 0;
    int i;

    for (i = 0; i < 7; ++i) {
        switch (selectors[i]) {
            case 0:
                acc += 10;
                break;

            case 1:
                acc += 20 + i;

            default:
                acc += 30 + selectors[i];
                if (selectors[i] == 5) {
                    break;
                }

            case 2:
                acc += 40 + i;
                if (selectors[i] == 2 && i > 0) {
                    goto after_switch;
                }
                break;

            case 3:
                acc += 50;

            case 4:
                acc += 60 + i;
                break;
        }

        acc += 100 + i;
after_switch:
        acc += 7;
    }

    printf("%d\n", acc);
    return 0;
}
